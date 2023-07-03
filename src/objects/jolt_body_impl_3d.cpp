#include "jolt_body_impl_3d.hpp"

#include "joints/jolt_joint_impl_3d.hpp"
#include "objects/jolt_area_impl_3d.hpp"
#include "objects/jolt_physics_direct_body_state_3d.hpp"
#include "servers/jolt_project_settings.hpp"
#include "spaces/jolt_broad_phase_layer.hpp"
#include "spaces/jolt_space_3d.hpp"

namespace {

template<typename TValue, typename TGetter>
bool integrate(TValue& p_value, PhysicsServer3D::AreaSpaceOverrideMode p_mode, TGetter&& p_getter) {
	switch (p_mode) {
		case PhysicsServer3D::AREA_SPACE_OVERRIDE_DISABLED: {
			return false;
		}
		case PhysicsServer3D::AREA_SPACE_OVERRIDE_COMBINE: {
			p_value += p_getter();
			return false;
		}
		case PhysicsServer3D::AREA_SPACE_OVERRIDE_COMBINE_REPLACE: {
			p_value += p_getter();
			return true;
		}
		case PhysicsServer3D::AREA_SPACE_OVERRIDE_REPLACE: {
			p_value = p_getter();
			return true;
		}
		case PhysicsServer3D::AREA_SPACE_OVERRIDE_REPLACE_COMBINE: {
			p_value = p_getter();
			return false;
		}
		default: {
			ERR_FAIL_D_MSG(vformat("Unhandled override mode: '%d'", p_mode));
		}
	}
}

} // namespace

JoltBodyImpl3D::~JoltBodyImpl3D() {
	memdelete_safely(direct_state);
}

Variant JoltBodyImpl3D::get_state(PhysicsServer3D::BodyState p_state) {
	switch (p_state) {
		case PhysicsServer3D::BODY_STATE_TRANSFORM: {
			return get_transform_scaled();
		}
		case PhysicsServer3D::BODY_STATE_LINEAR_VELOCITY: {
			return get_linear_velocity();
		}
		case PhysicsServer3D::BODY_STATE_ANGULAR_VELOCITY: {
			return get_angular_velocity();
		}
		case PhysicsServer3D::BODY_STATE_SLEEPING: {
			return is_sleeping();
		}
		case PhysicsServer3D::BODY_STATE_CAN_SLEEP: {
			return can_sleep();
		}
		default: {
			ERR_FAIL_D_MSG(vformat("Unhandled body state: '%d'", p_state));
		}
	}
}

void JoltBodyImpl3D::set_state(PhysicsServer3D::BodyState p_state, const Variant& p_value) {
	switch (p_state) {
		case PhysicsServer3D::BODY_STATE_TRANSFORM: {
			set_transform(p_value);
		} break;
		case PhysicsServer3D::BODY_STATE_LINEAR_VELOCITY: {
			set_linear_velocity(p_value);
		} break;
		case PhysicsServer3D::BODY_STATE_ANGULAR_VELOCITY: {
			set_angular_velocity(p_value);
		} break;
		case PhysicsServer3D::BODY_STATE_SLEEPING: {
			set_is_sleeping(p_value);
		} break;
		case PhysicsServer3D::BODY_STATE_CAN_SLEEP: {
			set_can_sleep(p_value);
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled body state: '%d'", p_state));
		} break;
	}
}

Variant JoltBodyImpl3D::get_param(PhysicsServer3D::BodyParameter p_param) const {
	switch (p_param) {
		case PhysicsServer3D::BODY_PARAM_BOUNCE: {
			return get_bounce();
		}
		case PhysicsServer3D::BODY_PARAM_FRICTION: {
			return get_friction();
		}
		case PhysicsServer3D::BODY_PARAM_MASS: {
			return get_mass();
		}
		case PhysicsServer3D::BODY_PARAM_INERTIA: {
			return get_inertia();
		}
		case PhysicsServer3D::BODY_PARAM_CENTER_OF_MASS: {
			return get_center_of_mass_custom();
		}
		case PhysicsServer3D::BODY_PARAM_GRAVITY_SCALE: {
			return get_gravity_scale();
		}
		case PhysicsServer3D::BODY_PARAM_LINEAR_DAMP_MODE: {
			return get_linear_damp_mode();
		}
		case PhysicsServer3D::BODY_PARAM_ANGULAR_DAMP_MODE: {
			return get_angular_damp_mode();
		}
		case PhysicsServer3D::BODY_PARAM_LINEAR_DAMP: {
			return get_linear_damp();
		}
		case PhysicsServer3D::BODY_PARAM_ANGULAR_DAMP: {
			return get_angular_damp();
		}
		default: {
			ERR_FAIL_D_MSG(vformat("Unhandled body parameter: '%d'", p_param));
		}
	}
}

void JoltBodyImpl3D::set_param(PhysicsServer3D::BodyParameter p_param, const Variant& p_value) {
	switch (p_param) {
		case PhysicsServer3D::BODY_PARAM_BOUNCE: {
			set_bounce(p_value);
		} break;
		case PhysicsServer3D::BODY_PARAM_FRICTION: {
			set_friction(p_value);
		} break;
		case PhysicsServer3D::BODY_PARAM_MASS: {
			set_mass(p_value);
		} break;
		case PhysicsServer3D::BODY_PARAM_INERTIA: {
			set_inertia(p_value);
		} break;
		case PhysicsServer3D::BODY_PARAM_CENTER_OF_MASS: {
			set_center_of_mass_custom(p_value);
		} break;
		case PhysicsServer3D::BODY_PARAM_GRAVITY_SCALE: {
			set_gravity_scale(p_value);
		} break;
		case PhysicsServer3D::BODY_PARAM_LINEAR_DAMP_MODE: {
			set_linear_damp_mode((DampMode)(int32_t)p_value);
		} break;
		case PhysicsServer3D::BODY_PARAM_ANGULAR_DAMP_MODE: {
			set_angular_damp_mode((DampMode)(int32_t)p_value);
		} break;
		case PhysicsServer3D::BODY_PARAM_LINEAR_DAMP: {
			set_linear_damp(p_value);
		} break;
		case PhysicsServer3D::BODY_PARAM_ANGULAR_DAMP: {
			set_angular_damp(p_value);
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled body parameter: '%d'", p_param));
		} break;
	}
}

JPH::BroadPhaseLayer JoltBodyImpl3D::get_broad_phase_layer() const {
	switch (mode) {
		case PhysicsServer3D::BODY_MODE_STATIC: {
			return JoltBroadPhaseLayer::BODY_STATIC;
		}
		case PhysicsServer3D::BODY_MODE_KINEMATIC:
		case PhysicsServer3D::BODY_MODE_RIGID:
		case PhysicsServer3D::BODY_MODE_RIGID_LINEAR: {
			return JoltBroadPhaseLayer::BODY_DYNAMIC;
		}
		default: {
			ERR_FAIL_D_MSG(vformat("Unhandled body mode: '%d'", mode));
		}
	}
}

void JoltBodyImpl3D::set_custom_integrator(bool p_enabled, bool p_lock) {
	custom_integrator = p_enabled;

	if (space == nullptr) {
		motion_changed(p_lock);
		return;
	}

	const JoltWritableBody3D body = space->write_body(jolt_id, p_lock);
	ERR_FAIL_COND(body.is_invalid());

	body->ResetForce();
	body->ResetTorque();

	JPH::MotionProperties& motion_properties = *body->GetMotionPropertiesUnchecked();

	if (custom_integrator) {
		motion_properties.SetLinearDamping(0.0f);
		motion_properties.SetAngularDamping(0.0f);
	} else {
		motion_properties.SetLinearDamping(total_linear_damp);
		motion_properties.SetAngularDamping(total_angular_damp);
	}

	motion_changed(false);
}

bool JoltBodyImpl3D::is_sleeping(bool p_lock) const {
	if (space == nullptr) {
		// HACK(mihe): Since `BODY_STATE_TRANSFORM` will be set right after creation it's more or
		// less impossible to have a body be sleeping when created, so we simply report this as not
		// sleeping.
		return false;
	}

	const JoltReadableBody3D body = space->read_body(jolt_id, p_lock);
	ERR_FAIL_COND_D(body.is_invalid());

	return !body->IsActive();
}

void JoltBodyImpl3D::set_is_sleeping(bool p_enabled, bool p_lock) {
	if (space == nullptr) {
		// HACK(mihe): Since `BODY_STATE_TRANSFORM` will be set right after creation it's more or
		// less impossible to have a body be sleeping when created, so we don't bother storing this.
		return;
	}

	JPH::BodyInterface& body_iface = space->get_body_iface(p_lock);

	if (p_enabled) {
		body_iface.DeactivateBody(jolt_id);
	} else {
		body_iface.ActivateBody(jolt_id);
	}
}

bool JoltBodyImpl3D::can_sleep(bool p_lock) const {
	if (space == nullptr) {
		return jolt_settings->mAllowSleeping;
	}

	const JoltReadableBody3D body = space->read_body(jolt_id, p_lock);
	ERR_FAIL_COND_D(body.is_invalid());

	return body->GetAllowSleeping();
}

void JoltBodyImpl3D::set_can_sleep(bool p_enabled, bool p_lock) {
	if (space == nullptr) {
		jolt_settings->mAllowSleeping = p_enabled;
		return;
	}

	const JoltWritableBody3D body = space->write_body(jolt_id, p_lock);
	ERR_FAIL_COND(body.is_invalid());

	body->SetAllowSleeping(p_enabled);
}

Basis JoltBodyImpl3D::get_principal_inertia_axes(bool p_lock) const {
	ERR_FAIL_NULL_D(space);

	if (is_static() || is_kinematic()) {
		return {};
	}

	const JoltReadableBody3D body = space->read_body(jolt_id, p_lock);
	ERR_FAIL_COND_D(body.is_invalid());

	// TODO(mihe): See if there's some way of getting this directly from Jolt

	Basis inertia_tensor = to_godot(jolt_shape->GetMassProperties().mInertia).basis;
	const Basis principal_inertia_axes_local = inertia_tensor.diagonalize().transposed();
	const Basis principal_inertia_axes = get_basis(false) * principal_inertia_axes_local;

	return principal_inertia_axes;
}

Vector3 JoltBodyImpl3D::get_inverse_inertia(bool p_lock) const {
	ERR_FAIL_NULL_D(space);

	if (is_static() || is_kinematic()) {
		return {};
	}

	const JoltReadableBody3D body = space->read_body(jolt_id, p_lock);
	ERR_FAIL_COND_D(body.is_invalid());

	return to_godot(body->GetMotionPropertiesUnchecked()->GetInverseInertiaDiagonal());
}

Basis JoltBodyImpl3D::get_inverse_inertia_tensor(bool p_lock) const {
	ERR_FAIL_NULL_D(space);

	if (is_static() || is_kinematic()) {
		return {};
	}

	const JoltReadableBody3D body = space->read_body(jolt_id, p_lock);
	ERR_FAIL_COND_D(body.is_invalid());

	return to_godot(body->GetInverseInertia()).basis;
}

void JoltBodyImpl3D::set_linear_velocity(const Vector3& p_velocity, bool p_lock) {
	if (is_static() || is_kinematic()) {
		linear_surface_velocity = p_velocity;
		motion_changed(p_lock);
		return;
	}

	if (space == nullptr) {
		jolt_settings->mLinearVelocity = to_jolt(p_velocity);
		motion_changed(p_lock);
		return;
	}

	const JoltWritableBody3D body = space->write_body(jolt_id, p_lock);
	ERR_FAIL_COND(body.is_invalid());

	body->GetMotionPropertiesUnchecked()->SetLinearVelocityClamped(to_jolt(p_velocity));

	motion_changed(false);
}

void JoltBodyImpl3D::set_angular_velocity(const Vector3& p_velocity, bool p_lock) {
	if (is_static() || is_kinematic()) {
		angular_surface_velocity = p_velocity;
		motion_changed(p_lock);
		return;
	}

	if (space == nullptr) {
		jolt_settings->mAngularVelocity = to_jolt(p_velocity);
		motion_changed(p_lock);
		return;
	}

	const JoltWritableBody3D body = space->write_body(jolt_id, p_lock);
	ERR_FAIL_COND(body.is_invalid());

	body->GetMotionPropertiesUnchecked()->SetAngularVelocityClamped(to_jolt(p_velocity));

	motion_changed(false);
}

void JoltBodyImpl3D::set_axis_velocity(const Vector3& p_axis_velocity, bool p_lock) {
	const Vector3 axis = p_axis_velocity.normalized();

	if (space == nullptr) {
		Vector3 linear_velocity = to_godot(jolt_settings->mLinearVelocity);
		linear_velocity -= axis * axis.dot(linear_velocity);
		linear_velocity += p_axis_velocity;
		jolt_settings->mLinearVelocity = to_jolt(linear_velocity);

		motion_changed(p_lock);
	} else {
		const JoltWritableBody3D body = space->write_body(jolt_id, p_lock);
		ERR_FAIL_COND(body.is_invalid());

		Vector3 linear_velocity = get_linear_velocity(false);
		linear_velocity -= axis * axis.dot(linear_velocity);
		linear_velocity += p_axis_velocity;
		set_linear_velocity(linear_velocity, false);

		motion_changed(false);
	}
}

void JoltBodyImpl3D::set_center_of_mass_custom(const Vector3& p_center_of_mass, bool p_lock) {
	if (custom_center_of_mass && p_center_of_mass == center_of_mass_custom) {
		return;
	}

	custom_center_of_mass = true;
	center_of_mass_custom = p_center_of_mass;

	build_shape(p_lock);
}

void JoltBodyImpl3D::add_contact(
	const JoltBodyImpl3D* p_collider,
	float p_depth,
	int32_t p_shape_index,
	int32_t p_collider_shape_index,
	const Vector3& p_normal,
	const Vector3& p_position,
	const Vector3& p_collider_position,
	const Vector3& p_collider_velocity,
	const Vector3& p_impulse
) {
	const int32_t max_contacts = get_max_contacts_reported();

	if (max_contacts == 0) {
		return;
	}

	Contact* contact = nullptr;

	if (contact_count < max_contacts) {
		contact = &contacts[contact_count++];
	} else {
		auto shallowest = std::min_element(
			contacts.begin(),
			contacts.end(),
			[](const Contact& p_lhs, const Contact& p_rhs) {
				return p_lhs.depth < p_rhs.depth;
			}
		);

		if (shallowest->depth < p_depth) {
			contact = &*shallowest;
		}
	}

	if (contact != nullptr) {
		contact->shape_index = p_shape_index;
		contact->collider_shape_index = p_collider_shape_index;
		contact->collider_id = p_collider->get_instance_id();
		contact->collider_rid = p_collider->get_rid();
		contact->normal = p_normal;
		contact->position = p_position;
		contact->collider_position = p_collider_position;
		contact->collider_velocity = p_collider_velocity;
		contact->impulse = p_impulse;
	}
}

void JoltBodyImpl3D::reset_mass_properties(bool p_lock) {
	inertia.zero();
	custom_center_of_mass = false;
	center_of_mass_custom.zero();
	update_mass_properties(p_lock);
}

void JoltBodyImpl3D::apply_force(const Vector3& p_force, const Vector3& p_position, bool p_lock) {
	ERR_FAIL_NULL(space);
	QUIET_FAIL_COND(!is_rigid());

	if (custom_integrator || p_force == Vector3()) {
		return;
	}

	const JoltWritableBody3D body = space->write_body(jolt_id, p_lock);
	ERR_FAIL_COND(body.is_invalid());

	body->AddForce(to_jolt(p_force), body->GetCenterOfMassPosition() + to_jolt(p_position));

	motion_changed(false);
}

void JoltBodyImpl3D::apply_central_force(const Vector3& p_force, bool p_lock) {
	ERR_FAIL_NULL(space);
	QUIET_FAIL_COND(!is_rigid());

	if (custom_integrator || p_force == Vector3()) {
		return;
	}

	const JoltWritableBody3D body = space->write_body(jolt_id, p_lock);
	ERR_FAIL_COND(body.is_invalid());

	body->AddForce(to_jolt(p_force));

	motion_changed(false);
}

void JoltBodyImpl3D::apply_impulse(
	const Vector3& p_impulse,
	const Vector3& p_position,
	bool p_lock
) {
	ERR_FAIL_NULL(space);
	QUIET_FAIL_COND(!is_rigid());

	if (p_impulse == Vector3()) {
		return;
	}

	const JoltWritableBody3D body = space->write_body(jolt_id, p_lock);
	ERR_FAIL_COND(body.is_invalid());

	body->AddImpulse(to_jolt(p_impulse), body->GetCenterOfMassPosition() + to_jolt(p_position));

	motion_changed(false);
}

void JoltBodyImpl3D::apply_central_impulse(const Vector3& p_impulse, bool p_lock) {
	ERR_FAIL_NULL(space);
	QUIET_FAIL_COND(!is_rigid());

	if (p_impulse == Vector3()) {
		return;
	}

	const JoltWritableBody3D body = space->write_body(jolt_id, p_lock);
	ERR_FAIL_COND(body.is_invalid());

	body->AddImpulse(to_jolt(p_impulse));

	motion_changed(false);
}

void JoltBodyImpl3D::apply_torque(const Vector3& p_torque, bool p_lock) {
	ERR_FAIL_NULL(space);
	QUIET_FAIL_COND(!is_rigid());

	if (custom_integrator || p_torque == Vector3()) {
		return;
	}

	const JoltWritableBody3D body = space->write_body(jolt_id, p_lock);
	ERR_FAIL_COND(body.is_invalid());

	body->AddTorque(to_jolt(p_torque));

	motion_changed(false);
}

void JoltBodyImpl3D::apply_torque_impulse(const Vector3& p_impulse, bool p_lock) {
	ERR_FAIL_NULL(space);
	QUIET_FAIL_COND(!is_rigid());

	if (p_impulse == Vector3()) {
		return;
	}

	const JoltWritableBody3D body = space->write_body(jolt_id, p_lock);
	ERR_FAIL_COND(body.is_invalid());

	body->AddAngularImpulse(to_jolt(p_impulse));

	motion_changed(false);
}

void JoltBodyImpl3D::add_constant_central_force(const Vector3& p_force, bool p_lock) {
	if (p_force == Vector3()) {
		return;
	}

	constant_force += p_force;

	motion_changed(p_lock);
}

void JoltBodyImpl3D::add_constant_force(
	const Vector3& p_force,
	const Vector3& p_position,
	bool p_lock
) {
	if (p_force == Vector3()) {
		return;
	}

	const JoltWritableBody3D body = space->write_body(jolt_id, p_lock);
	ERR_FAIL_COND(body.is_invalid());

	const Vector3 center_of_mass = get_center_of_mass(false);
	const Vector3 body_position = get_position(false);
	const Vector3 center_of_mass_relative = center_of_mass - body_position;

	constant_force += p_force;
	constant_torque += (p_position - center_of_mass_relative).cross(p_force);

	motion_changed(false);
}

void JoltBodyImpl3D::add_constant_torque(const Vector3& p_torque, bool p_lock) {
	if (p_torque == Vector3()) {
		return;
	}

	constant_torque += p_torque;

	motion_changed(p_lock);
}

Vector3 JoltBodyImpl3D::get_constant_force() const {
	return constant_force;
}

void JoltBodyImpl3D::set_constant_force(const Vector3& p_force, bool p_lock) {
	if (constant_force == p_force) {
		return;
	}

	constant_force = p_force;

	motion_changed(p_lock);
}

Vector3 JoltBodyImpl3D::get_constant_torque() const {
	return constant_torque;
}

void JoltBodyImpl3D::set_constant_torque(const Vector3& p_torque, bool p_lock) {
	if (constant_torque == p_torque) {
		return;
	}

	constant_torque = p_torque;

	motion_changed(p_lock);
}

void JoltBodyImpl3D::add_collision_exception(const RID& p_excepted_body, bool p_lock) {
	if (group_filter == nullptr) {
		group_filter = new JoltGroupFilterRID();
	}

	group_filter->add_exception(p_excepted_body);

	exceptions_changed(p_lock);
}

void JoltBodyImpl3D::remove_collision_exception(const RID& p_excepted_body, bool p_lock) {
	if (group_filter == nullptr) {
		return;
	}

	group_filter->remove_exception(p_excepted_body);

	if (group_filter->get_exception_count() == 0) {
		group_filter = nullptr;
	}

	exceptions_changed(p_lock);
}

bool JoltBodyImpl3D::has_collision_exception(const RID& p_excepted_body) const {
	return group_filter != nullptr && group_filter->has_exception(p_excepted_body);
}

TypedArray<RID> JoltBodyImpl3D::get_collision_exceptions() const {
	if (group_filter == nullptr) {
		return {};
	}

	const RID* exceptions = group_filter->get_exceptions();
	const int32_t exception_count = group_filter->get_exception_count();

	TypedArray<RID> result;

	for (auto i = 0; i < exception_count; ++i) {
		result.push_back(exceptions[i]);
	}

	return result;
}

void JoltBodyImpl3D::add_area(JoltAreaImpl3D* p_area, bool p_lock) {
	areas.ordered_insert(p_area, [](const JoltAreaImpl3D* p_lhs, const JoltAreaImpl3D* p_rhs) {
		return p_lhs->get_priority() > p_rhs->get_priority();
	});

	areas_changed(p_lock);
}

void JoltBodyImpl3D::remove_area(JoltAreaImpl3D* p_area, bool p_lock) {
	areas.erase(p_area);

	areas_changed(p_lock);
}

void JoltBodyImpl3D::add_joint(JoltJointImpl3D* p_joint, bool p_lock) {
	joints.push_back(p_joint);

	joints_changed(p_lock);
}

void JoltBodyImpl3D::remove_joint(JoltJointImpl3D* p_joint, bool p_lock) {
	joints.erase(p_joint);

	joints_changed(p_lock);
}

void JoltBodyImpl3D::call_queries([[maybe_unused]] JPH::Body& p_jolt_body) {
	if (is_rigid() && custom_integration_callback.is_valid()) {
		static thread_local Array arguments = []() {
			Array array;
			array.resize(2);
			return array;
		}();

		arguments[0] = get_direct_state();
		arguments[1] = custom_integration_userdata;

		custom_integration_callback.callv(arguments);
	}

	if (sync_state && body_state_callback.is_valid()) {
		static thread_local Array arguments = []() {
			Array array;
			array.resize(1);
			return array;
		}();

		arguments[0] = get_direct_state();

		body_state_callback.callv(arguments);

		sync_state = false;
	}
}

void JoltBodyImpl3D::pre_step(float p_step, JPH::Body& p_jolt_body) {
	JoltObjectImpl3D::pre_step(p_step, p_jolt_body);

	switch (mode) {
		case PhysicsServer3D::BODY_MODE_STATIC: {
			pre_step_static(p_step, p_jolt_body);
		} break;
		case PhysicsServer3D::BODY_MODE_RIGID:
		case PhysicsServer3D::BODY_MODE_RIGID_LINEAR: {
			pre_step_rigid(p_step, p_jolt_body);
		} break;
		case PhysicsServer3D::BODY_MODE_KINEMATIC: {
			pre_step_kinematic(p_step, p_jolt_body);
		} break;
	}

	contact_count = 0;
}

void JoltBodyImpl3D::move_kinematic(float p_step, JPH::Body& p_jolt_body) {
	p_jolt_body.SetLinearVelocity(JPH::Vec3::sZero());
	p_jolt_body.SetAngularVelocity(JPH::Vec3::sZero());

	const JPH::Vec3 current_position = p_jolt_body.GetPosition();
	const JPH::Quat current_rotation = p_jolt_body.GetRotation();

	const JPH::Vec3 new_position = to_jolt(kinematic_transform.origin);
	const JPH::Quat new_rotation = to_jolt(kinematic_transform.basis);

	if (new_position == current_position && new_rotation == current_rotation) {
		return;
	}

	p_jolt_body.MoveKinematic(new_position, new_rotation, p_step);

	sync_state = true;
}

JoltPhysicsDirectBodyState3D* JoltBodyImpl3D::get_direct_state() {
	if (direct_state == nullptr) {
		direct_state = memnew(JoltPhysicsDirectBodyState3D(this));
	}

	return direct_state;
}

void JoltBodyImpl3D::set_mode(PhysicsServer3D::BodyMode p_mode, bool p_lock) {
	if (p_mode == mode) {
		return;
	}

	mode = p_mode;

	if (space == nullptr) {
		mode_changed(p_lock);
		return;
	}

	const JPH::EMotionType motion_type = get_motion_type();

	const JoltWritableBody3D body = space->write_body(jolt_id, p_lock);
	ERR_FAIL_COND(body.is_invalid());

	if (motion_type == JPH::EMotionType::Static) {
		put_to_sleep(false);
	}

	body->SetMotionType(motion_type);

	if (motion_type != JPH::EMotionType::Static) {
		wake_up(false);
	}

	if (motion_type == JPH::EMotionType::Kinematic) {
		body->SetLinearVelocity(JPH::Vec3::sZero());
		body->SetAngularVelocity(JPH::Vec3::sZero());
	}

	linear_surface_velocity = Vector3();
	angular_surface_velocity = Vector3();

	mode_changed(false);
}

bool JoltBodyImpl3D::is_ccd_enabled(bool p_lock) const {
	if (space == nullptr) {
		return jolt_settings->mMotionQuality == JPH::EMotionQuality::LinearCast;
	}

	const JPH::BodyInterface& body_iface = space->get_body_iface(p_lock);

	return body_iface.GetMotionQuality(jolt_id) == JPH::EMotionQuality::LinearCast;
}

void JoltBodyImpl3D::set_ccd_enabled(bool p_enabled, bool p_lock) {
	const JPH::EMotionQuality motion_quality = p_enabled
		? JPH::EMotionQuality::LinearCast
		: JPH::EMotionQuality::Discrete;

	if (space == nullptr) {
		jolt_settings->mMotionQuality = motion_quality;
		return;
	}

	JPH::BodyInterface& body_iface = space->get_body_iface(p_lock);

	body_iface.SetMotionQuality(jolt_id, motion_quality);
}

void JoltBodyImpl3D::set_mass(float p_mass, bool p_lock) {
	if (p_mass != mass) {
		mass = p_mass;
		update_mass_properties(p_lock);
	}
}

void JoltBodyImpl3D::set_inertia(const Vector3& p_inertia, bool p_lock) {
	if (p_inertia != inertia) {
		inertia = p_inertia;
		update_mass_properties(p_lock);
	}
}

float JoltBodyImpl3D::get_bounce(bool p_lock) const {
	if (space == nullptr) {
		return jolt_settings->mRestitution;
	}

	const JoltReadableBody3D body = space->read_body(jolt_id, p_lock);
	ERR_FAIL_COND_D(body.is_invalid());

	return body->GetRestitution();
}

void JoltBodyImpl3D::set_bounce(float p_bounce, bool p_lock) {
	if (space == nullptr) {
		jolt_settings->mRestitution = p_bounce;
		return;
	}

	const JoltWritableBody3D body = space->write_body(jolt_id, p_lock);
	ERR_FAIL_COND(body.is_invalid());

	body->SetRestitution(p_bounce);
}

float JoltBodyImpl3D::get_friction(bool p_lock) const {
	if (space == nullptr) {
		return jolt_settings->mFriction;
	}

	const JoltReadableBody3D body = space->read_body(jolt_id, p_lock);
	ERR_FAIL_COND_D(body.is_invalid());

	return body->GetFriction();
}

void JoltBodyImpl3D::set_friction(float p_friction, bool p_lock) {
	if (space == nullptr) {
		jolt_settings->mFriction = p_friction;
		return;
	}

	const JoltWritableBody3D body = space->write_body(jolt_id, p_lock);
	ERR_FAIL_COND(body.is_invalid());

	body->SetFriction(p_friction);
}

float JoltBodyImpl3D::get_gravity_scale(bool p_lock) const {
	if (space == nullptr) {
		return jolt_settings->mGravityFactor;
	}

	const JoltReadableBody3D body = space->read_body(jolt_id, p_lock);
	ERR_FAIL_COND_D(body.is_invalid());

	return body->GetMotionPropertiesUnchecked()->GetGravityFactor();
}

void JoltBodyImpl3D::set_gravity_scale(float p_scale, bool p_lock) {
	if (space == nullptr) {
		jolt_settings->mGravityFactor = p_scale;
		return;
	}

	const JoltWritableBody3D body = space->write_body(jolt_id, p_lock);
	ERR_FAIL_COND(body.is_invalid());

	body->GetMotionPropertiesUnchecked()->SetGravityFactor(p_scale);

	motion_changed(false);
}

void JoltBodyImpl3D::set_linear_damp(float p_damp, bool p_lock) {
	if (p_damp < 0.0f) {
		WARN_PRINT(vformat(
			"Invalid linear damp for '%s'. "
			"Linear damp values less than 0 are not supported by Godot Jolt. "
			"Values outside this range will be clamped.",
			to_string()
		));

		p_damp = 0;
	}

	if (p_damp == linear_damp) {
		return;
	}

	linear_damp = p_damp;

	update_damp(p_lock);
}

void JoltBodyImpl3D::set_angular_damp(float p_damp, bool p_lock) {
	if (p_damp < 0.0f) {
		WARN_PRINT(vformat(
			"Invalid angular damp for '%s'. "
			"Angular damp values less than 0 are not supported by Godot Jolt. "
			"Values outside this range will be clamped.",
			to_string()
		));

		p_damp = 0;
	}

	if (p_damp == angular_damp) {
		return;
	}

	angular_damp = p_damp;

	update_damp(p_lock);
}

bool JoltBodyImpl3D::is_axis_locked(PhysicsServer3D::BodyAxis p_axis) const {
	return (locked_axes & (uint32_t)p_axis) != 0;
}

void JoltBodyImpl3D::set_axis_lock(
	PhysicsServer3D::BodyAxis p_axis,
	bool p_lock_axis,
	bool p_lock
) {
	const uint32_t previous_locked_axes = locked_axes;

	if (p_lock_axis) {
		locked_axes |= (uint32_t)p_axis;
	} else {
		locked_axes &= ~(uint32_t)p_axis;
	}

	if (previous_locked_axes != locked_axes) {
		axis_lock_changed(p_lock);
	}
}

JPH::EMotionType JoltBodyImpl3D::get_motion_type() const {
	switch (mode) {
		case PhysicsServer3D::BODY_MODE_STATIC: {
			return JPH::EMotionType::Static;
		}
		case PhysicsServer3D::BODY_MODE_KINEMATIC: {
			return JPH::EMotionType::Kinematic;
		}
		case PhysicsServer3D::BODY_MODE_RIGID:
		case PhysicsServer3D::BODY_MODE_RIGID_LINEAR: {
			return JPH::EMotionType::Dynamic;
		}
		default: {
			ERR_FAIL_D_MSG(vformat("Unhandled body mode: '%d'", mode));
		}
	}
}

void JoltBodyImpl3D::create_in_space() {
	create_begin();

	jolt_settings->mAllowDynamicOrKinematic = true;
	jolt_settings->mMaxLinearVelocity = JoltProjectSettings::get_max_linear_velocity();
	jolt_settings->mMaxAngularVelocity = JoltProjectSettings::get_max_angular_velocity();

	// HACK(mihe): We need to defer the setting of mass properties, to allow for overriding the
	// inverse inertia for `BODY_MODE_RIGID_LINEAR`, which we can't do until the body is created, so
	// we set it to some random values and calculate it properly once the body is created instead.
	jolt_settings->mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
	jolt_settings->mMassPropertiesOverride.mMass = 1.0f;
	jolt_settings->mMassPropertiesOverride.mInertia = JPH::Mat44::sIdentity();

	JPH::Body* body = create_end();
	ERR_FAIL_NULL(body);

	// HACK(mihe): Since group filters don't grant us access to user data we are instead forced
	// abuse the collision group to carry the upper and lower bits of our RID, which we can then
	// access and rebuild in our group filter for bodies that make use of collision exceptions.

	JPH::CollisionGroup::GroupID group_id = 0;
	JPH::CollisionGroup::SubGroupID sub_group_id = 0;
	JoltGroupFilterRID::encode_rid(rid, group_id, sub_group_id);

	body->SetCollisionGroup(JPH::CollisionGroup(nullptr, group_id, sub_group_id));
}

void JoltBodyImpl3D::integrate_forces(float p_step, JPH::Body& p_jolt_body) {
	if (!p_jolt_body.IsActive()) {
		return;
	}

	gravity = Vector3();

	const Vector3 position = to_godot(p_jolt_body.GetPosition());

	bool gravity_done = false;

	for (const JoltAreaImpl3D* area : areas) {
		gravity_done = integrate(gravity, area->get_gravity_mode(), [&]() {
			return area->compute_gravity(position);
		});

		if (gravity_done) {
			break;
		}
	}

	if (!gravity_done) {
		gravity += space->get_default_area()->compute_gravity(position);
	}

	JPH::MotionProperties& motion_properties = *p_jolt_body.GetMotionPropertiesUnchecked();

	gravity *= motion_properties.GetGravityFactor();

	if (!custom_integrator) {
		motion_properties.SetLinearVelocityClamped(
			motion_properties.GetLinearVelocity() + to_jolt(gravity) * p_step
		);

		p_jolt_body.AddForce(to_jolt(constant_force));
		p_jolt_body.AddTorque(to_jolt(constant_torque));
	}

	sync_state = true;
}

void JoltBodyImpl3D::pre_step_static(
	[[maybe_unused]] float p_step,
	[[maybe_unused]] JPH::Body& p_jolt_body
) {
	// Nothing to do
}

void JoltBodyImpl3D::pre_step_rigid(float p_step, JPH::Body& p_jolt_body) {
	integrate_forces(p_step, p_jolt_body);
}

void JoltBodyImpl3D::pre_step_kinematic(float p_step, JPH::Body& p_jolt_body) {
	move_kinematic(p_step, p_jolt_body);

	if (generates_contacts()) {
		// HACK(mihe): This seems to emulate the behavior of Godot Physics, where kinematic bodies
		// are set as active (and thereby have their state synchronized on every step) only if its
		// max reported contacts is non-zero.
		sync_state = true;
	}
}

void JoltBodyImpl3D::apply_transform(const Transform3D& p_transform, bool p_lock) {
	if (is_kinematic()) {
		kinematic_transform = p_transform;
	}

	if (!is_kinematic() || space == nullptr) {
		JoltObjectImpl3D::apply_transform(p_transform, p_lock);
	}
}

JPH::MassProperties JoltBodyImpl3D::calculate_mass_properties(const JPH::Shape& p_shape) const {
	const bool calculate_mass = mass <= 0;
	const bool calculate_inertia = inertia.x <= 0 || inertia.y <= 0 || inertia.z <= 0;

	JPH::MassProperties mass_properties = p_shape.GetMassProperties();

	if (calculate_mass && calculate_inertia) {
		// Use the mass properties calculated by the shape
	} else if (calculate_inertia) {
		mass_properties.ScaleToMass(mass);
	} else {
		mass_properties.mMass = mass;
		mass_properties.mInertia(0, 0) = inertia.x;
		mass_properties.mInertia(1, 1) = inertia.y;
		mass_properties.mInertia(2, 2) = inertia.z;
	}

	mass_properties.mInertia(3, 3) = 1.0f;

	return mass_properties;
}

JPH::MassProperties JoltBodyImpl3D::calculate_mass_properties() const {
	return calculate_mass_properties(*jolt_shape);
}

void JoltBodyImpl3D::update_mass_properties(bool p_lock) {
	if (space == nullptr) {
		return;
	}

	const JoltWritableBody3D body = space->write_body(jolt_id, p_lock);
	ERR_FAIL_COND(body.is_invalid());

	JPH::MotionProperties& motion_properties = *body->GetMotionPropertiesUnchecked();

	motion_properties.SetMassProperties(calculate_mass_properties());

	if (is_rigid_linear()) {
		motion_properties.SetInverseInertia(JPH::Vec3::sZero(), JPH::Quat::sIdentity());
	}
}

void JoltBodyImpl3D::update_damp(bool p_lock) {
	if (space == nullptr) {
		return;
	}

	total_linear_damp = 0.0;
	total_angular_damp = 0.0;

	bool linear_damp_done = linear_damp_mode == PhysicsServer3D::BODY_DAMP_MODE_REPLACE;
	bool angular_damp_done = angular_damp_mode == PhysicsServer3D::BODY_DAMP_MODE_REPLACE;

	for (const JoltAreaImpl3D* area : areas) {
		if (!linear_damp_done) {
			linear_damp_done = integrate(total_linear_damp, area->get_linear_damp_mode(), [&]() {
				return area->get_linear_damp();
			});
		}

		if (!angular_damp_done) {
			angular_damp_done = integrate(total_angular_damp, area->get_angular_damp_mode(), [&]() {
				return area->get_angular_damp();
			});
		}

		if (linear_damp_done && angular_damp_done) {
			break;
		}
	}

	const JoltAreaImpl3D* default_area = space->get_default_area();

	if (!linear_damp_done) {
		total_linear_damp += default_area->get_linear_damp();
	}

	if (!angular_damp_done) {
		total_angular_damp += default_area->get_angular_damp();
	}

	switch (linear_damp_mode) {
		case PhysicsServer3D::BODY_DAMP_MODE_COMBINE: {
			total_linear_damp += linear_damp;
		} break;
		case PhysicsServer3D::BODY_DAMP_MODE_REPLACE: {
			total_linear_damp = linear_damp;
		} break;
	}

	switch (angular_damp_mode) {
		case PhysicsServer3D::BODY_DAMP_MODE_COMBINE: {
			total_angular_damp += angular_damp;
		} break;
		case PhysicsServer3D::BODY_DAMP_MODE_REPLACE: {
			total_angular_damp = angular_damp;
		} break;
	}

	const JoltWritableBody3D jolt_body = space->write_body(jolt_id, p_lock);
	ERR_FAIL_COND(jolt_body.is_invalid());

	if (!custom_integrator) {
		JPH::MotionProperties& motion_properties = *jolt_body->GetMotionPropertiesUnchecked();

		motion_properties.SetLinearDamping(total_linear_damp);
		motion_properties.SetAngularDamping(total_angular_damp);
	}

	motion_changed(false);
}

void JoltBodyImpl3D::update_kinematic_transform(bool p_lock) {
	if (is_kinematic()) {
		kinematic_transform = get_transform_unscaled(p_lock);
	}
}

void JoltBodyImpl3D::update_joint_constraints(bool p_lock) {
	for (JoltJointImpl3D* joint : joints) {
		joint->rebuild(p_lock);
	}
}

void JoltBodyImpl3D::destroy_joint_constraints() {
	for (JoltJointImpl3D* joint : joints) {
		joint->destroy();
	}
}

void JoltBodyImpl3D::update_axes_constraint(bool p_lock) {
	if (space == nullptr) {
		return;
	}

	destroy_axes_constraint();

	const bool locked_linear_x = is_axis_locked(PhysicsServer3D::BODY_AXIS_LINEAR_X);
	const bool locked_linear_y = is_axis_locked(PhysicsServer3D::BODY_AXIS_LINEAR_Y);
	const bool locked_linear_z = is_axis_locked(PhysicsServer3D::BODY_AXIS_LINEAR_Z);

	bool locked_angular_x = is_axis_locked(PhysicsServer3D::BODY_AXIS_ANGULAR_X);
	bool locked_angular_y = is_axis_locked(PhysicsServer3D::BODY_AXIS_ANGULAR_Y);
	bool locked_angular_z = is_axis_locked(PhysicsServer3D::BODY_AXIS_ANGULAR_Z);

	if (is_rigid_linear()) {
		// HACK(mihe): The infinite inertia that's used for `BODY_MODE_RIGID_LINEAR` doesn't seem to
		// play very nicely with angular constraints, resulting in a bunch of NaNs, so we ignore
		// those locks in that case.
		locked_angular_x = false;
		locked_angular_y = false;
		locked_angular_z = false;
	}

	const bool locked_linear = locked_linear_x || locked_linear_y || locked_linear_z;
	const bool locked_angular = locked_angular_x || locked_angular_y || locked_angular_z;

	if (!locked_linear && !locked_angular) {
		return;
	}

	const JoltWritableBody3D jolt_body = space->write_body(jolt_id, p_lock);
	ERR_FAIL_COND(jolt_body.is_invalid());

	JPH::SixDOFConstraintSettings constraint_settings;
	constraint_settings.mPosition1 = jolt_body->GetCenterOfMassPosition();
	constraint_settings.mPosition2 = constraint_settings.mPosition1;

	if (locked_linear_x) {
		constraint_settings.MakeFixedAxis(JPH::SixDOFConstraintSettings::EAxis::TranslationX);
	}

	if (locked_linear_y) {
		constraint_settings.MakeFixedAxis(JPH::SixDOFConstraintSettings::EAxis::TranslationY);
	}

	if (locked_linear_z) {
		constraint_settings.MakeFixedAxis(JPH::SixDOFConstraintSettings::EAxis::TranslationZ);
	}

	if (locked_angular_x) {
		constraint_settings.MakeFixedAxis(JPH::SixDOFConstraintSettings::EAxis::RotationX);
	}

	if (locked_angular_y) {
		constraint_settings.MakeFixedAxis(JPH::SixDOFConstraintSettings::EAxis::RotationY);
	}

	if (locked_angular_z) {
		constraint_settings.MakeFixedAxis(JPH::SixDOFConstraintSettings::EAxis::RotationZ);
	}

	axes_constraint = constraint_settings.Create(JPH::Body::sFixedToWorld, *jolt_body);

	space->add_joint(axes_constraint);
}

void JoltBodyImpl3D::destroy_axes_constraint() {
	if (space != nullptr && axes_constraint != nullptr) {
		space->remove_joint(axes_constraint);
		axes_constraint = nullptr;
	}
}

void JoltBodyImpl3D::update_group_filter(bool p_lock) {
	if (space == nullptr) {
		return;
	}

	const JoltWritableBody3D body = space->write_body(jolt_id, p_lock);
	ERR_FAIL_COND(body.is_invalid());

	body->GetCollisionGroup().SetGroupFilter(group_filter);
}

void JoltBodyImpl3D::mode_changed(bool p_lock) {
	update_object_layer(p_lock);
	update_kinematic_transform(p_lock);
	update_mass_properties(p_lock);
	update_axes_constraint(p_lock);
	wake_up(p_lock);
}

void JoltBodyImpl3D::shapes_built(bool p_lock) {
	update_mass_properties(p_lock);
	wake_up(p_lock);
}

void JoltBodyImpl3D::space_changing([[maybe_unused]] bool p_lock) {
	destroy_joint_constraints();
	destroy_axes_constraint();
}

void JoltBodyImpl3D::space_changed(bool p_lock) {
	update_mass_properties(p_lock);
	update_group_filter(p_lock);
	update_joint_constraints(p_lock);
	update_axes_constraint(p_lock);
	areas_changed(p_lock);
}

void JoltBodyImpl3D::areas_changed(bool p_lock) {
	update_damp(p_lock);
	wake_up(p_lock);
}

void JoltBodyImpl3D::joints_changed(bool p_lock) {
	wake_up(p_lock);
}

void JoltBodyImpl3D::transform_changed(bool p_lock) {
	wake_up(p_lock);
}

void JoltBodyImpl3D::motion_changed(bool p_lock) {
	wake_up(p_lock);
}

void JoltBodyImpl3D::exceptions_changed(bool p_lock) {
	update_group_filter(p_lock);
}

void JoltBodyImpl3D::axis_lock_changed(bool p_lock) {
	update_axes_constraint(p_lock);
	wake_up(p_lock);
}
