#include "jolt_shaped_object_impl_3d.hpp"

#include "shapes/jolt_custom_empty_shape.hpp"
#include "shapes/jolt_shape_impl_3d.hpp"
#include "spaces/jolt_space_3d.hpp"

JoltShapedObjectImpl3D::JoltShapedObjectImpl3D(ObjectType p_object_type)
	: JoltObjectImpl3D(p_object_type) {
	jolt_settings->mAllowSleeping = true;
	jolt_settings->mFriction = 1.0f;
	jolt_settings->mRestitution = 0.0f;
	jolt_settings->mLinearDamping = 0.0f;
	jolt_settings->mAngularDamping = 0.0f;
	jolt_settings->mGravityFactor = 0.0f;
}

JoltShapedObjectImpl3D::~JoltShapedObjectImpl3D() {
	delete_safely(jolt_settings);
}

Transform3D JoltShapedObjectImpl3D::get_transform_unscaled() const {
	if (space == nullptr) {
		return {to_godot(jolt_settings->mRotation), to_godot(jolt_settings->mPosition)};
	}

	const JoltReadableBody3D body = space->read_body(jolt_id);
	ERR_FAIL_COND_D(body.is_invalid());

	return {to_godot(body->GetRotation()), to_godot(body->GetPosition())};
}

Transform3D JoltShapedObjectImpl3D::get_transform_scaled() const {
	return get_transform_unscaled().scaled_local(scale);
}

Basis JoltShapedObjectImpl3D::get_basis() const {
	if (space == nullptr) {
		return to_godot(jolt_settings->mRotation);
	}

	const JoltReadableBody3D body = space->read_body(jolt_id);
	ERR_FAIL_COND_D(body.is_invalid());

	return to_godot(body->GetRotation());
}

Vector3 JoltShapedObjectImpl3D::get_position() const {
	if (space == nullptr) {
		return to_godot(jolt_settings->mPosition);
	}

	const JoltReadableBody3D body = space->read_body(jolt_id);
	ERR_FAIL_COND_D(body.is_invalid());

	return to_godot(body->GetPosition());
}

Vector3 JoltShapedObjectImpl3D::get_center_of_mass() const {
	ERR_FAIL_NULL_D_MSG(
		space,
		vformat(
			"Failed to retrieve center-of-mass of '%s'. "
			"Doing so without a physics space is not supported by Godot Jolt. "
			"If this relates to a node, try adding the node to a scene tree first.",
			to_string()
		)
	);

	const JoltReadableBody3D body = space->read_body(jolt_id);
	ERR_FAIL_COND_D(body.is_invalid());

	return to_godot(body->GetCenterOfMassPosition());
}

Vector3 JoltShapedObjectImpl3D::get_center_of_mass_local() const {
	ERR_FAIL_NULL_D_MSG(
		space,
		vformat(
			"Failed to retrieve local center-of-mass of '%s'. "
			"Doing so without a physics space is not supported by Godot Jolt. "
			"If this relates to a node, try adding the node to a scene tree first.",
			to_string()
		)
	);

	return get_transform_scaled().xform_inv(get_center_of_mass());
}

Vector3 JoltShapedObjectImpl3D::get_linear_velocity() const {
	if (space == nullptr) {
		return to_godot(jolt_settings->mLinearVelocity);
	}

	const JoltReadableBody3D body = space->read_body(jolt_id);
	ERR_FAIL_COND_D(body.is_invalid());

	return to_godot(body->GetLinearVelocity());
}

Vector3 JoltShapedObjectImpl3D::get_angular_velocity() const {
	if (space == nullptr) {
		return to_godot(jolt_settings->mAngularVelocity);
	}

	const JoltReadableBody3D body = space->read_body(jolt_id);
	ERR_FAIL_COND_D(body.is_invalid());

	return to_godot(body->GetAngularVelocity());
}

JPH::ShapeRefC JoltShapedObjectImpl3D::try_build_shape() {
	int32_t built_shape_count = 0;
	const JoltShapeInstance3D* last_built_shape = nullptr;

	for (JoltShapeInstance3D& shape : shapes) {
		if (shape.is_enabled() && shape.try_build()) {
			built_shape_count += 1;
			last_built_shape = &shape;
		}
	}

	if (built_shape_count == 0) {
		return {};
	}

	JPH::ShapeRefC result;

	if (built_shape_count == 1) {
		result = JoltShapeImpl3D::with_transform(
			last_built_shape->get_jolt_ref(),
			last_built_shape->get_transform_unscaled(),
			last_built_shape->get_scale()
		);
	} else {
		int32_t shape_index = 0;

		result = JoltShapeImpl3D::as_compound([&](auto&& p_add_shape) {
			if (shape_index >= shapes.size()) {
				return false;
			}

			const JoltShapeInstance3D& shape = shapes[shape_index++];

			if (shape.is_enabled() && shape.is_built()) {
				p_add_shape(
					shape.get_jolt_ref(),
					shape.get_transform_unscaled(),
					shape.get_scale()
				);
			}

			return true;
		});
	}

	if (has_custom_center_of_mass()) {
		result = JoltShapeImpl3D::with_center_of_mass(result, get_center_of_mass_custom());
	}

	if (scale != Vector3(1.0f, 1.0f, 1.0f)) {
		result = JoltShapeImpl3D::with_scale(result, scale);
	}

	return result;
}

JPH::ShapeRefC JoltShapedObjectImpl3D::build_shape() {
	JPH::ShapeRefC new_shape = try_build_shape();

	if (new_shape == nullptr) {
		new_shape = new JoltCustomEmptyShape();
	}

	return new_shape;
}

void JoltShapedObjectImpl3D::update_shape() {
	if (space == nullptr) {
		_shapes_built();
		return;
	}

	const JoltWritableBody3D body = space->write_body(jolt_id);
	ERR_FAIL_COND(body.is_invalid());

	previous_jolt_shape = jolt_shape;
	jolt_shape = build_shape();

	if (jolt_shape == previous_jolt_shape) {
		return;
	}

	space->get_body_iface().SetShape(jolt_id, jolt_shape, false, JPH::EActivation::DontActivate);

	_shapes_built();
}

void JoltShapedObjectImpl3D::add_shape(
	JoltShapeImpl3D* p_shape,
	Transform3D p_transform,
	bool p_disabled
) {
	Vector3 shape_scale;
	Math::decompose(p_transform, shape_scale);

	shapes.emplace_back(this, p_shape, p_transform, shape_scale, p_disabled);

	_shapes_changed();
}

void JoltShapedObjectImpl3D::remove_shape(const JoltShapeImpl3D* p_shape) {
	shapes.erase_if([&](const JoltShapeInstance3D& p_instance) {
		return p_instance.get_shape() == p_shape;
	});

	_shapes_changed();
}

void JoltShapedObjectImpl3D::remove_shape(int32_t p_index) {
	ERR_FAIL_INDEX(p_index, shapes.size());

	shapes.remove_at(p_index);

	_shapes_changed();
}

JoltShapeImpl3D* JoltShapedObjectImpl3D::get_shape(int32_t p_index) const {
	ERR_FAIL_INDEX_D(p_index, shapes.size());

	return shapes[p_index].get_shape();
}

void JoltShapedObjectImpl3D::set_shape(int32_t p_index, JoltShapeImpl3D* p_shape) {
	ERR_FAIL_INDEX(p_index, shapes.size());

	shapes[p_index] = JoltShapeInstance3D(this, p_shape);

	_shapes_changed();
}

void JoltShapedObjectImpl3D::clear_shapes() {
	shapes.clear();

	_shapes_changed();
}

int32_t JoltShapedObjectImpl3D::find_shape_index(uint32_t p_shape_instance_id) const {
	return shapes.find_if([&](const JoltShapeInstance3D& p_shape) {
		return p_shape.get_id() == p_shape_instance_id;
	});
}

int32_t JoltShapedObjectImpl3D::find_shape_index(const JPH::SubShapeID& p_sub_shape_id) const {
	ERR_FAIL_NULL_V(jolt_shape, -1);

	return find_shape_index((uint32_t)jolt_shape->GetSubShapeUserData(p_sub_shape_id));
}

JoltShapeImpl3D* JoltShapedObjectImpl3D::find_shape(uint32_t p_shape_instance_id) const {
	const int32_t shape_index = find_shape_index(p_shape_instance_id);
	return shape_index != -1 ? shapes[shape_index].get_shape() : nullptr;
}

JoltShapeImpl3D* JoltShapedObjectImpl3D::find_shape(const JPH::SubShapeID& p_sub_shape_id) const {
	const int32_t shape_index = find_shape_index(p_sub_shape_id);
	return shape_index != -1 ? shapes[shape_index].get_shape() : nullptr;
}

Transform3D JoltShapedObjectImpl3D::get_shape_transform_unscaled(int32_t p_index) const {
	ERR_FAIL_INDEX_D(p_index, shapes.size());

	return shapes[p_index].get_transform_unscaled();
}

Transform3D JoltShapedObjectImpl3D::get_shape_transform_scaled(int32_t p_index) const {
	ERR_FAIL_INDEX_D(p_index, shapes.size());

	return shapes[p_index].get_transform_scaled();
}

Vector3 JoltShapedObjectImpl3D::get_shape_scale(int32_t p_index) const {
	ERR_FAIL_INDEX_D(p_index, shapes.size());

	return shapes[p_index].get_scale();
}

void JoltShapedObjectImpl3D::set_shape_transform(int32_t p_index, Transform3D p_transform) {
	ERR_FAIL_INDEX(p_index, shapes.size());

#ifdef DEBUG_ENABLED
	ERR_FAIL_COND_MSG(
		p_transform.basis.determinant() == 0.0f,
		vformat(
			"Failed to set transform for shape at index %d of body '%s'. "
			"The basis was found to be singular, which is not supported by Godot Jolt. "
			"This is likely caused by one or more axes having a scale of zero.",
			p_index,
			to_string()
		)
	);
#endif // DEBUG_ENABLED

	Vector3 new_scale;
	Math::decompose(p_transform, new_scale);

	JoltShapeInstance3D& shape = shapes[p_index];

	if (shape.get_transform_unscaled() == p_transform && shape.get_scale() == new_scale) {
		return;
	}

	shape.set_transform(p_transform);
	shape.set_scale(new_scale);

	_shapes_changed();
}

bool JoltShapedObjectImpl3D::is_shape_disabled(int32_t p_index) const {
	ERR_FAIL_INDEX_D(p_index, shapes.size());

	return shapes[p_index].is_disabled();
}

void JoltShapedObjectImpl3D::set_shape_disabled(int32_t p_index, bool p_disabled) {
	ERR_FAIL_INDEX(p_index, shapes.size());

	JoltShapeInstance3D& shape = shapes[p_index];

	if (shape.is_disabled() == p_disabled) {
		return;
	}

	if (p_disabled) {
		shape.disable();
	} else {
		shape.enable();
	}

	_shapes_changed();
}

void JoltShapedObjectImpl3D::post_step(float p_step, JPH::Body& p_jolt_body) {
	JoltObjectImpl3D::post_step(p_step, p_jolt_body);

	previous_jolt_shape = nullptr;
}

void JoltShapedObjectImpl3D::_shapes_changed() {
	update_shape();
}

void JoltShapedObjectImpl3D::_space_changing() {
	JoltObjectImpl3D::_space_changing();

	if (space != nullptr) {
		const JoltWritableBody3D body = space->write_body(jolt_id);
		ERR_FAIL_COND(body.is_invalid());

		jolt_settings = new JPH::BodyCreationSettings(body->GetBodyCreationSettings());
	}
}
