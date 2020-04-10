/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file physics_character_controller.h
 * @author Brian Lach
 * @date December 26, 2019
 */

#ifndef PHYSICS_CHARACTER_CONTROLLER_H
#define PHYSICS_CHARACTER_CONTROLLER_H

#include <bulletWorld.h>
#include <bulletCapsuleShape.h>
#include <bulletRigidBodyNode.h>
#include <nodePath.h>
#include <nodePathCollection.h>
#include <aa_luse.h>
#include <py_panda.h>

#include "config_bsp.h"

class BSPLoader;

BEGIN_PUBLISH
enum MovementState
{
	MOVEMENTSTATE_GROUND,
	MOVEMENTSTATE_JUMPING,
	MOVEMENTSTATE_FALLING,
	MOVEMENTSTATE_SWIMMING,
};
END_PUBLISH

struct ContactData
{
	BulletRigidBodyNode *node;
	LVector3 hit_pos;
	LVector3 hit_normal;
	bool has_contact;

	ContactData()
	{
		node = nullptr;
		has_contact = false;
	}

	void set_contact( BulletRigidBodyNode *n, const LVector3 &hp, const LVector3 &hn )
	{
		node = n;
		hit_pos = hp;
		hit_normal = hn;
		has_contact = true;
	}

	void clear_contact()
	{
		has_contact = false;
	}
};

struct CapsuleData
{
	float height;
	float levitation;
	float radius;
	PT( BulletCapsuleShape ) capsule;
	PT( BulletRigidBodyNode ) capsule_node;
	NodePath capsule_np;
};

class EXPCL_PANDABSP PhysicsCharacterController : public ReferenceCount
{
PUBLISHED:
	PhysicsCharacterController( BSPLoader *loader, BulletWorld *world, const NodePath &render,
				    const NodePath &parent, float walk_height,
				    float crouch_height, float step_height, float radius,
				    float gravity, const BitMask32 &wall_mask, const BitMask32 &floor_mask,
				    const BitMask32 &event_mask );

	void set_max_slope( float degs, bool affect_speed );

	void set_collide_mask( const BitMask32 &mask );

#ifdef HAVE_PYTHON
	INLINE void set_event_enter_callback( PyObject *callback )
	{
		if ( _event_enter_callback )
			Py_DECREF( _event_enter_callback );
		Py_INCREF( callback );
		_event_enter_callback = callback;
	}

	INLINE void set_event_exit_callback( PyObject *callback )
	{
		if ( _event_exit_callback )
			Py_DECREF( _event_exit_callback );
		Py_INCREF( callback );
		_event_exit_callback = callback;
	}
#endif

	INLINE void set_active_jump_limiter( bool limiter )
	{
		_intelligent_jump = limiter;
	}

	INLINE void set_default_material( const std::string &mat )
	{
		_default_material = mat;
	}

	INLINE void set_touching_water( bool flag )
	{
		_touching_water = flag;
	}

	INLINE std::string get_current_material() const
	{
		return _current_material;
	}

	INLINE NodePath get_walk_capsule() const
	{
		return _walk_capsule_data.capsule_np;
	}

	INLINE NodePath get_crouch_capsule() const
	{
		return _crouch_capsule_data.capsule_np;
	}

	INLINE NodePath get_event_sphere() const
	{
		return _event_sphere_np;
	}

	INLINE NodePath get_capsule() const
	{
		return _capsule_data->capsule_np;
	}

	INLINE NodePath get_movement_parent() const
	{
		return _movement_parent;
	}

	INLINE void set_gravity( float grav )
	{
		_gravity = grav;
	}

	INLINE void set_movement_state( int state )
	{
		_movement_state = state;
	}
	INLINE int get_movement_state() const
	{
		return _movement_state;
	}

#ifdef HAVE_PYTHON
	INLINE void set_stand_up_callback( PyObject *callback )
	{
		if ( _stand_up_callback )
			Py_DECREF( _stand_up_callback );
		Py_INCREF( callback );
		_stand_up_callback = callback;
	}

	INLINE void set_fall_callback( PyObject *callback )
	{
		if ( _fall_callback )
			Py_DECREF( _fall_callback );
		Py_INCREF( callback );
		_fall_callback = callback;
	}
#endif

	void start_crouch();
	void stop_crouch();

	bool is_on_ground();

	void start_jump( float max_height = 3.0f );

	void set_angular_movement( float omega );
	void set_linear_movement( const LVector3 &movement );

	void place_on_ground();

	void update( float frametime );

	void remove_capsules();

private:
	void update_event_sphere();
	void update_eye_ray();
	void update_foot_contact();
	void update_head_contact();
	void update_capsule();
	void apply_linear_velocity();
	void prevent_penetration();
	void setup( float walk_height, float crouch_height, float step_height, float radius );
	void add_elements();
	void land();
	void fall();
	void stand_up();
	void jump( float max_z = 3.0f );
	void process_ground();
	void process_falling();
	void process_jumping();
	void process_swimming();
	bool check_future_space( const LVector3 &global_vel );

	void apply_gravity( const LVector3 &floor_normal );

	void setup_capsule( CapsuleData *data, bool attach );

private:
	BulletWorld *_world;
	BitMask32 _wall_mask;
	BitMask32 _floor_mask;
	BitMask32 _event_mask;
	NodePath _render;
	NodePath _parent;
	float _timestep;
	LVector3 _target_pos;
	LVector3 _current_pos;

	CapsuleData _walk_capsule_data;
	CapsuleData _crouch_capsule_data;
	
	CapsuleData *_capsule_data;

	float _capsule_offset;
	float _capsule_top;
	float _foot_distance;

	float _height;
	float _radius;
	float _walk_height;
	float _crouch_height;
	float _step_height;
	bool _above_ground;
	bool _no_clip;
	NodePath _movement_parent;
	float _gravity;
	int _movement_state;
	bool _predict_future_space;
	float _future_space_prediction_distance;
	bool _is_crouching;
	float _fall_time;
	float _fall_delta;
	float _fall_start_pos;
	LVector3 _linear_velocity;
	ContactData _head_contact;
	ContactData _foot_contact;
	bool _enabled_crouch;
	float _min_slope_dot;
	bool _slope_affects_speed;
	bool _intelligent_jump;
	float _jump_time;
	float _jump_speed;
	float _jump_max_height;
	float _jump_start_pos;

	PT( BulletGhostNode ) _event_sphere;
	NodePath _event_sphere_np;

	NodePathCollection _prev_overlapping;

	// The brush material we are standing on.
	std::string _current_material;
	std::string _default_material;
	bool _touching_water;

	BSPLoader *_bsp_loader;

#ifdef HAVE_PYTHON
	PyObject *_stand_up_callback;
	PyObject *_fall_callback;
	PyObject *_event_enter_callback;
	PyObject *_event_exit_callback;
#endif
};

#endif // PHYSICS_CHARACTER_CONTROLLER_H