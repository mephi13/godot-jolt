#pragma once

#include "jolt_space_3d.hpp"
#include "jolt_physics_direct_space_state_3d.hpp"

class JoltStateRecorder final : public Object {
    GDCLASS(JoltStateRecorder, Object)

public:
	explicit JoltStateRecorder();
	~JoltStateRecorder();

    void SetValidating(bool inValidating);
    bool IsValidating();
    JPH::StateRecorder& GetRecorder() { return *internal_recorder; };

    void SaveState(JoltPhysicsDirectSpaceState3D* JoltSpace);
	bool RestoreState(JoltPhysicsDirectSpaceState3D* JoltSpace);

protected:
	static void _bind_methods();

private:
	JPH::StateRecorder* internal_recorder;
};