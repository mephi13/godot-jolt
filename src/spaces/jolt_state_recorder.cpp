#include "jolt_state_recorder.hpp"


JoltStateRecorder::JoltStateRecorder() {
	internal_recorder = new JPH::StateRecorderImpl();
}

JoltStateRecorder::~JoltStateRecorder() {
	delete_safely(internal_recorder);
}

void JoltStateRecorder::SetValidating(bool inValidating) {
    internal_recorder->SetValidating(inValidating);
}

bool JoltStateRecorder::IsValidating() {
    return internal_recorder->IsValidating();
}

void JoltStateRecorder::SaveState(JoltPhysicsDirectSpaceState3D* JoltSpace) {
	JoltSpace->get_space().get_physics_system().SaveState(*internal_recorder);
}

bool JoltStateRecorder::RestoreState(JoltPhysicsDirectSpaceState3D* JoltSpace) {
    if (internal_recorder->IsEOF() || ((JPH::StateRecorderImpl*)internal_recorder)->IsFailed()) {
        return false;
    }
	return JoltSpace->get_space().get_physics_system().RestoreState(*internal_recorder);
}

void JoltStateRecorder::_bind_methods() {
    ClassDB::bind_method(D_METHOD("is_validating"), &JoltStateRecorder::IsValidating);
    ClassDB::bind_method(D_METHOD("set_validating", "inValidating"), &JoltStateRecorder::SetValidating);
    ClassDB::bind_method(D_METHOD("save_state", "physics_system"), &JoltStateRecorder::SaveState);
    ClassDB::bind_method(D_METHOD("restore_state", "physics_system"), &JoltStateRecorder::RestoreState);
}