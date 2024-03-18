extends RigidBody3D

var recorder = JoltStateRecorder.new()
var previous_state = JoltStateRecorder.new()
var next_state = JoltStateRecorder.new()
var first_frame = true
var state;
var acc_time = 0.0;
@onready
var rid : RID = get_world_3d().space;

# Called when the node enters the scene tree for the first time.
func _ready():
	JoltPhysicsServer3D.space_set_active(rid, false);
	state = get_world_3d().get_direct_space_state();
	recorder.save_state(state)	

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _physics_process(delta):
	pass

func _process(delta):
	var frame_time = 1.0/60.0
	
	if Input.is_action_pressed("forward"):
		acc_time += delta
		if acc_time > frame_time:
			acc_time -= frame_time
		
			JoltPhysicsServer3D.space_step(rid, frame_time)
			JoltPhysicsServer3D.space_flush_queries(rid)

	if first_frame:
		first_frame = false
	if Input.is_action_just_pressed("reset"):
		#recorder.restore_state(state)
		var sum = 0;
		for item in get_parent().get_children():
			if item is Node3D:
				sum += hash(item.transform)
		# end state should be 23367139142
		print(sum)

