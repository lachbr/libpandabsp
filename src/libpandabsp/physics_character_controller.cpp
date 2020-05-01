/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file physics_character_controller.cpp
 * @author Brian Lach
 * @date December 26, 2019
 */

#include "physics_character_controller.h"

#include <deg_2_rad.h>
#include <bulletSphereShape.h>
#include <eventHandler.h>

#include "bsploader.h"

PhysicsCharacterController::PhysicsCharacterController( BSPLoader *loader, BulletWorld *world, const NodePath &render,
							const NodePath &parent, float walk_height,
							float crouch_height, float step_height, float radius,
							float gravity, const BitMask32 &wall_mask, const BitMask32 &floor_mask,
							const BitMask32 &event_mask )
{
	_bsp_loader = loader;
	_world = world;
	_render = render;
	_parent = parent;
	_walk_height = walk_height;
	_crouch_height = crouch_height;
	_step_height = step_height;
	_radius = radius;
	_gravity = gravity;
	_wall_mask = wall_mask;
	_floor_mask = floor_mask;
	_event_mask = event_mask;
	_foot_contact.clear_contact();
	_head_contact.clear_contact();
	_touching_water = false;
	_current_material = "default";
	_default_material = "default";
	_fall_time = 0.0f;
	_fall_start_pos = 0.0f;
	_fall_delta = 0.0f;
	_predict_future_space = true;
	_future_space_prediction_distance = 10.0f;
	_is_crouching = false;
	_enabled_crouch = false;
	_no_clip = false;
	_above_ground = true;
	_timestep = 0.0f;
	_movement_state = MOVEMENTSTATE_GROUND;
	_intelligent_jump = true;
#ifdef HAVE_PYTHON
	_stand_up_callback = nullptr;
	_fall_callback = nullptr;
	_event_enter_callback = nullptr;
	_event_exit_callback = nullptr;
#endif
	_jump_time = 0.0f;
	_jump_speed = 0.0f;
	_jump_max_height = 0.0f;
	_jump_start_pos = 0.0f;
	_event_sphere = nullptr;

	_movement_parent = parent.attach_new_node( "physicsMovementParent" );
	setup( _walk_height, _crouch_height, _step_height, _radius );
	set_max_slope( 50.0f, true );
}

static void set_data( CapsuleData *data, float fullh, float steph, float r )
{
	float length;
	float lev;
	if ( fullh - steph <= r * 2.0f )
	{
		length = 0.1f;
		r = ( fullh * 0.5f ) - ( steph * 0.5f );
		lev = steph;
	}
	else
	{
		length = fullh - ( r * 2.0f );
		lev = r + steph;
	}

	data->height = length;
	data->levitation = lev;
	data->radius = r;
}

void PhysicsCharacterController::setup( float walk_height, float crouch_height, float step_height, float radius )
{
	_walk_height = walk_height;
	_crouch_height = crouch_height;
	_radius = radius;

	set_data( &_walk_capsule_data, walk_height, step_height, radius );
	set_data( &_crouch_capsule_data, crouch_height, step_height, radius );

	_capsule_data = &_walk_capsule_data;
	_height = _walk_height;
	_capsule_offset = _capsule_data->height * 0.5f + _capsule_data->levitation;
	_foot_distance = 3.0f;//_capsule_offset + _capsule_data->levitation;

	add_elements();
}

void PhysicsCharacterController::setup_capsule( CapsuleData *data, bool attach )
{
	// Setup walk capsule
	data->capsule = new BulletCapsuleShape( data->radius, data->height );
	data->capsule_node = new BulletRigidBodyNode( "capsule" );
	data->capsule_np = _movement_parent.attach_new_node( data->capsule_node );
	data->capsule_node->add_shape( data->capsule );
	data->capsule_node->set_kinematic( true );
	data->capsule_node->set_ccd_motion_threshold( 1e-7 );
	data->capsule_node->set_ccd_swept_sphere_radius( data->radius );
	data->capsule_np.set_collide_mask( BitMask32::all_on() );

	if ( attach )
		_world->attach( data->capsule_node );
}

void PhysicsCharacterController::add_elements()
{
	
	setup_capsule( &_walk_capsule_data, true );
	setup_capsule( &_crouch_capsule_data, false );

	PT( BulletSphereShape ) event_sphere = new BulletSphereShape( _walk_capsule_data.radius * 1.5f );
	_event_sphere = new BulletGhostNode( "event_sphere" );
	_event_sphere->add_shape( event_sphere, TransformState::make_pos( LPoint3( 0, 0, _walk_capsule_data.radius * 1.5f ) ) );
	_event_sphere->set_kinematic( true );
	_event_sphere_np = _movement_parent.attach_new_node( _event_sphere );
	_event_sphere_np.set_collide_mask( BitMask32::all_off() );
	_world->attach( _event_sphere );

	// Init
	update_capsule();
}

void PhysicsCharacterController::set_max_slope( float degs, bool affects_speed )
{
	_min_slope_dot = std::cosf( deg_2_rad( degs ) );
	_slope_affects_speed = affects_speed;
}

void PhysicsCharacterController::set_angular_movement( float omega )
{
	_movement_parent.set_h( _movement_parent, omega );
}

void PhysicsCharacterController::set_linear_movement( const LVector3 &speed )
{
	_linear_velocity = speed;
}

void PhysicsCharacterController::place_on_ground()
{
	land();
	_linear_velocity = LVector3::zero();
}

void PhysicsCharacterController::update( float frametime )
{
	_current_pos = _movement_parent.get_pos( _render );
	_target_pos = LVector3( _current_pos );

	// Check if there is a ground below us.
	LPoint3 from = _capsule_data->capsule_np.get_pos( _render ) + LPoint3( 0, 0, 0.1f );
	LPoint3 to = from - LPoint3( 0, 0, 2000 );
	BulletClosestHitRayResult result = _world->ray_test_closest( from, to, _floor_mask );
	// Only fall is there is a ground for us to fall onto.
	// Prevents the character from falling out of the world.
	_above_ground = result.has_hit();

	_timestep = frametime;

	update_event_sphere();
	update_eye_ray();
	update_foot_contact();
	update_head_contact();

	switch ( _movement_state )
	{
	case MOVEMENTSTATE_GROUND:
		process_ground();
		break;
	case MOVEMENTSTATE_JUMPING:
		process_jumping();
		break;
	case MOVEMENTSTATE_FALLING:
		process_falling();
		break;
	case MOVEMENTSTATE_SWIMMING:
		process_swimming();
		break;
	}

	apply_linear_velocity();
	prevent_penetration();

	update_capsule();

	if ( _is_crouching && !_enabled_crouch )
		stand_up();
}

void PhysicsCharacterController::apply_gravity( const LVector3 &normal )
{
	_target_pos -= LVector3( normal[0], normal[1], 0.0f ) * _gravity * _timestep * 0.1f;
}

void PhysicsCharacterController::apply_linear_velocity()
{
	LVector3 global_vel = _linear_velocity * _timestep;

	if ( _predict_future_space && !check_future_space( global_vel ) )
		return;

	if ( _foot_contact.has_contact && _min_slope_dot != -1 && _movement_state != MOVEMENTSTATE_SWIMMING )
	{
		LVector3 normal_vel( global_vel );
		normal_vel.normalize();

		LVector3 floor_normal = _foot_contact.hit_normal;

		float abs_slope_dot = floor_normal.dot( LVector3::up() );
		if ( abs_slope_dot <= _min_slope_dot )
		{
			apply_gravity( floor_normal );

			if ( global_vel != LVector3::zero() )
			{
				LVector3 global_vel_dir( global_vel );
				global_vel_dir.normalize();
				LVector3 fn( floor_normal[0], floor_normal[1], 0.0f );
				fn.normalize();
				float veldot = 1.0f - global_vel_dir.angle_deg( fn ) / 180.0f;
				if ( veldot < 0.5f )
					_target_pos -= LVector3( fn[0] * global_vel[0], fn[1] * global_vel[1], 0.0f ) * veldot;
				global_vel *= veldot;
			}
		}
		else if ( _slope_affects_speed && global_vel != LVector3::zero() )
		{
			apply_gravity( floor_normal );
		}
	}
	else if ( _movement_state == MOVEMENTSTATE_SWIMMING && !is_on_ground() )
	{
		_target_pos -= LVector3( 0, 0, -_gravity * _timestep * 0.1f );
	}

	_target_pos += global_vel;
}

static LVector3 reflect( const LVector3 &vec, const LVector3 &normal )
{
	return vec - ( normal * ( vec.dot( normal ) * 2.0f ) );
}

static LVector3 parallel( const LVector3 &vec, const LVector3 &normal )
{
	return normal * vec.dot( normal );
}

static LVector3 perpendicular( const LVector3 &vec, const LVector3 &normal )
{
	return vec - parallel( vec, normal );
}

//#define NEW_METHOD

struct ShoveData
{
	LVector3 vector;
	float length;
	bool valid;
	BulletContact contact;
};

void PhysicsCharacterController::prevent_penetration()
{
	if ( _no_clip )
		return;

#ifndef NEW_METHOD
	int max_itr = 10;
	float fraction = 1.0f;

	LVector3 collisions( 0 );
	LPoint3 offset( 0, 0, _capsule_offset );

	float old_margin = _capsule_data->capsule->get_margin();
	_capsule_data->capsule->set_margin( _capsule_data->capsule->get_margin() + 0.02f );

	while ( fraction > 0.01f && max_itr > 0 )
	{
		LPoint3 current_target = _target_pos + collisions;
		CPT( TransformState ) from = TransformState::make_pos( _current_pos + offset );
		CPT( TransformState ) to = TransformState::make_pos( current_target + offset );
		BulletClosestHitSweepResult result = _world->sweep_test_closest( _capsule_data->capsule, *from, *to, _wall_mask, 1e-7 );
		if ( result.has_hit() && !result.get_node()->is_of_type( BulletGhostNode::get_class_type() ) )
		{
			BulletRigidBodyNode *node = DCAST( BulletRigidBodyNode, result.get_node() );
			if ( node->get_collision_response() )
			{
				fraction -= result.get_hit_fraction();
				LVector3 normal = result.get_hit_normal();
				LVector3 direction = result.get_to_pos() - result.get_from_pos();
				float distance = direction.length();
				direction.normalize();
				if ( distance != 0.0f )
				{
					LVector3 refl_dir = reflect( direction, normal );
					refl_dir.normalize();
					LVector3 coll_dir = parallel( refl_dir, normal );
					collisions += coll_dir * distance;
				}
			}
		}

		max_itr -= 1;
	}
	_capsule_data->capsule->set_margin( old_margin );
	collisions[2] = 0.0f;
	_target_pos += collisions;
#else // NEW_METHOD
	epvector<ShoveData> shoves;
	BulletContactResult result = _world->contact_test( _capsule_data->capsule_node );
	for ( int i = 0; i < result.get_num_contacts(); i++ )
	{
		BulletContact contact = result.get_contact( i );
		PandaNode *nodeb = contact.get_node1();
		if ( ( nodeb->get_into_collide_mask() & _wall_mask ) == 0 || !nodeb->is_of_type( BulletRigidBodyNode::get_class_type() ) )
			continue;
		LPoint3 surface_point;
		LVector3 normal;
		LPoint3 interior_point;

		BulletManifoldPoint manifold = contact.get_manifold_point();
		surface_point = manifold.get_position_world_on_b();
		normal = manifold.get_normal_world_on_b();
		interior_point = manifold.get_position_world_on_a();

		if ( manifold.get_distance() > 0.0f )
		{
			LVector3 nd = _linear_velocity.project( normal );
			_linear_velocity -= nd * 1.05f;
		}
	}
#endif // NEW_METHOD
}

bool PhysicsCharacterController::check_future_space( const LVector3 &gv )
{
	LVector3 global_vel = gv * _future_space_prediction_distance;
	LPoint3 from = _capsule_data->capsule_np.get_pos( _render ) + global_vel;
	LPoint3 up = from + LPoint3( 0, 0, _capsule_data->height * 2.0f );
	LPoint3 down = from - LPoint3( 0, 0, _capsule_data->height * 2.0f + _capsule_data->levitation );

	BulletClosestHitRayResult up_test = _world->ray_test_closest( from, up, _wall_mask );
	BulletClosestHitRayResult down_test = _world->ray_test_closest( from, down, _floor_mask );

	if ( !( up_test.has_hit() && down_test.has_hit() ) )
		return true;

	BulletRigidBodyNode *up_node = DCAST( BulletRigidBodyNode, up_test.get_node() );
	if ( up_node->get_mass() > 0.0f )
		return true;

	float space = std::fabsf( up_test.get_hit_pos()[2] - down_test.get_hit_pos()[2] );

	if ( space < _capsule_data->levitation + _capsule_data->height + _capsule_data->radius )
		return false;

	return true;
}

void PhysicsCharacterController::update_capsule()
{
	_movement_parent.set_pos( _render, _target_pos );
	_capsule_data->capsule_np.set_pos( 0, 0, _capsule_offset );

	_capsule_top = _target_pos[2] + _capsule_data->levitation + _capsule_data->height * 2.0f;
}

void PhysicsCharacterController::update_eye_ray()
{
	// TODO
}

void PhysicsCharacterController::update_event_sphere()
{
	EventHandler *evh = EventHandler::get_global_event_handler();

	NodePathCollection overlapping;

	BulletContactResult result = _world->contact_test( _event_sphere );
	for ( int i = 0; i < result.get_num_contacts(); i++ )
	{
		BulletContact contact = result.get_contact( i );
		PandaNode *node = contact.get_node1();
		NodePath np( node );

		if ( !_prev_overlapping.has_path( np ) )
		{
			// The avatar has entered this node.
#ifdef HAVE_PYTHON
			if ( _event_enter_callback )
			{
				// Tell python code about it.
				PyObject *py_np =
					DTool_CreatePyInstance( &np, *(Dtool_PyTypedObject *)np.get_class_type().get_python_type(), true, true );
				Py_INCREF( py_np );
				PyObject *args = PyTuple_Pack( 1, py_np );
				PyObject_CallObject( _event_enter_callback, args );
			}
#endif
		}

		overlapping.add_path( np );
	}

	for ( int i = 0; i < _prev_overlapping.get_num_paths(); i++ )
	{
		NodePath np = _prev_overlapping[i];
		if ( !overlapping.has_path( np ) )
		{
			// The avatar has exited this node.
#ifdef HAVE_PYTHON
			if ( _event_exit_callback )
			{
				// Tell python code about it.
				PyObject *py_np =
					DTool_CreatePyInstance( &np, *(Dtool_PyTypedObject *)np.get_class_type().get_python_type(), true, true );
				Py_INCREF( py_np );
				PyObject *args = PyTuple_Pack( 1, py_np );
				PyObject_CallObject( _event_exit_callback, args );
			}
#endif
		}
	}

	// Remember who we overlapped with.
	_prev_overlapping = overlapping;
}

void PhysicsCharacterController::update_foot_contact()
{
	BSPLoader *loader = _bsp_loader;

	if ( !_above_ground )
	{
		// If we aren't above a ground, check if we are below a ground.
		// If we are below a ground, snap up to the ground.

		LPoint3 from = _capsule_data->capsule_np.get_pos( _render );
		LPoint3 to = from + LPoint3( 0, 0, 2000 );
		BulletClosestHitRayResult result = _world->ray_test_closest( from, to, _floor_mask );
		if ( result.has_hit() )
		{
			// We are below a ground, snap ourselves up to it.
			_foot_contact.set_contact( DCAST( BulletRigidBodyNode, result.get_node() ),
						   result.get_hit_pos(), result.get_hit_normal() );
			_target_pos[2] = _foot_contact.hit_pos[2];
			_movement_state = MOVEMENTSTATE_GROUND;
			return;
		}
	}

	LPoint3 from = _capsule_data->capsule_np.get_pos( _render );
	LPoint3 to = from - LPoint3( 0, 0, _foot_distance );
	BulletAllHitsRayResult result = _world->ray_test_all( from, to, _wall_mask | _floor_mask );
	if ( !result.has_hits() )
	{
		_foot_contact.clear_contact();
		return;
	}

	pvector<BulletRayHit> sorted_hits;

	for ( int i = 0; i < result.get_num_hits(); i++ )
	{
		BulletRayHit hit = result.get_hit( i );
		sorted_hits.push_back( hit );
	}

	std::sort( sorted_hits.begin(), sorted_hits.end(), []( const BulletRayHit &a, const BulletRayHit &b )
	{
		return ( a.get_hit_fraction() < b.get_hit_fraction() );
	} );

	for ( auto itr = sorted_hits.begin(); itr != sorted_hits.end(); itr++ )
	{
		BulletRayHit hit = *itr;
		if ( hit.get_node()->is_of_type( BulletGhostNode::get_class_type() ) )
			continue;

		BulletRigidBodyNode *node = DCAST( BulletRigidBodyNode, hit.get_node() );

		int triangle_idx = hit.get_triangle_index();
		if ( _movement_state != MOVEMENTSTATE_SWIMMING && !_touching_water )
		{
			std::string mat = _default_material;
			if ( loader->has_brush_collision_node( node ) )
			{
				if ( loader->has_brush_collision_triangle( node, triangle_idx ) )
				{
					mat = loader->get_brush_triangle_material( node, triangle_idx );
				}
			}
			_current_material = mat;
		}

		_foot_contact.set_contact( node, hit.get_hit_pos(), hit.get_hit_normal() );
		break;
	}
}

void PhysicsCharacterController::update_head_contact()
{
	LPoint3 from = _capsule_data->capsule_np.get_pos( _render );
	LPoint3 to = from + LPoint3( 0, 0, _capsule_data->height * 20.0f );
	BulletAllHitsRayResult result = _world->ray_test_all( from, to, _wall_mask );

	if ( !result.has_hits() )
	{
		_head_contact.clear_contact();
		return;
	}

	pvector<BulletRayHit> sorted_hits;

	for ( int i = 0; i < result.get_num_hits(); i++ )
	{
		BulletRayHit hit = result.get_hit( i );
		sorted_hits.push_back( hit );
	}

	std::sort( sorted_hits.begin(), sorted_hits.end(), []( const BulletRayHit &a, const BulletRayHit &b )
	{
		return ( a.get_hit_fraction() < b.get_hit_fraction() );
	} );

	for ( auto itr = sorted_hits.begin(); itr != sorted_hits.end(); itr++ )
	{
		BulletRayHit hit = *itr;
		if ( hit.get_node()->is_of_type( BulletGhostNode::get_class_type() ) )
			continue;

		BulletRigidBodyNode *node = DCAST( BulletRigidBodyNode, hit.get_node() );
		_head_contact.set_contact( node, hit.get_hit_pos(), hit.get_hit_normal() );
		break;
	}
}

void PhysicsCharacterController::start_crouch()
{
	if ( _enabled_crouch )
		return;

	_is_crouching = true;
	_enabled_crouch = true;

	_capsule_data = &_crouch_capsule_data;
	_height = _crouch_height;
	_world->remove( _walk_capsule_data.capsule_node );
	_world->attach( _crouch_capsule_data.capsule_node );

	_capsule_offset = _capsule_data->height * 0.5f + _capsule_data->levitation;
	_foot_distance = 3.0f;//_capsule_offset + _capsule_data->levitation;
}

void PhysicsCharacterController::stop_crouch()
{
	_enabled_crouch = false;
}

bool PhysicsCharacterController::is_on_ground()
{
	if ( !_foot_contact.has_contact )
		return false;
	else if ( _movement_state == MOVEMENTSTATE_GROUND )
	{
		float elevation = _target_pos[2] - _foot_contact.hit_pos[2];
		return elevation <= 0.02f;
	}

	return _target_pos[2] <= _foot_contact.hit_pos[2];
}

void PhysicsCharacterController::start_jump( float max_height )
{
	jump( max_height );
}

void PhysicsCharacterController::jump( float max_height )
{
	if ( _movement_state != MOVEMENTSTATE_GROUND )
		return;

	max_height += _target_pos[2];

	if ( _intelligent_jump && _head_contact.has_contact && _head_contact.hit_pos[2] < max_height + _height )
		max_height = _head_contact.hit_pos[2] - _height * 1.2f;

	_jump_start_pos = _target_pos[2];
	_jump_time = 0.0f;
	float bsq = -4.0f * _gravity * ( max_height - _jump_start_pos );
	float b = std::sqrtf( bsq );
	_jump_speed = b;
	_jump_max_height = max_height;

	_movement_state = MOVEMENTSTATE_JUMPING;

	EventHandler *evh = EventHandler::get_global_event_handler();
	evh->dispatch_event( new Event( "jumpStart" ) );
}

void PhysicsCharacterController::stand_up()
{
	update_head_contact();

	if ( _head_contact.has_contact )
	{
		float z = _target_pos[2] + _walk_capsule_data.levitation + _walk_capsule_data.height;
		if ( z >= _head_contact.hit_pos[2] )
			return;
	}

	_is_crouching = false;
	_capsule_data = &_walk_capsule_data;
	_height = _walk_height;

	_world->remove( _crouch_capsule_data.capsule_node );
	_world->attach( _walk_capsule_data.capsule_node );

	_capsule_offset = _capsule_data->height * 0.5f + _capsule_data->levitation;
	_foot_distance = 3.0f;//_capsule_offset + _capsule_data->levitation;

#ifdef HAVE_PYTHON
	if ( _stand_up_callback != nullptr )
	{
		PyObject *args = PyTuple_New( 0 );
		PyObject_CallObject( _stand_up_callback, args );
	}
#endif
}

void PhysicsCharacterController::land()
{
	_movement_state = MOVEMENTSTATE_GROUND;
}

void PhysicsCharacterController::fall()
{
	_movement_state = MOVEMENTSTATE_FALLING;

	_fall_start_pos = _target_pos[2];
	_fall_time = 0.0f;
	_fall_delta = 0.0f;
}

void PhysicsCharacterController::remove_capsules()
{
	_world->remove( _walk_capsule_data.capsule_node );
	_world->remove( _crouch_capsule_data.capsule_node );
	_world->remove( _event_sphere );
}

void PhysicsCharacterController::process_falling()
{
	if ( !_above_ground )
	{
		// Don't fall if we don't have a ground to fall onto!
		return;
	}

	_fall_time += _timestep;
	_fall_delta = _gravity * std::powf( _fall_time, 2.0f );

	LPoint3 new_pos( _target_pos );
	new_pos[2] = _fall_start_pos + _fall_delta;

	_target_pos = new_pos;

	if ( is_on_ground() )
	{
		land();
		// Put the avatar on the ground right now instead of waiting
		// until the next frame.
		_target_pos[2] = _foot_contact.hit_pos[2];

#ifdef HAVE_PYTHON
		if ( _fall_callback != nullptr )
		{
			PyObject *args = PyTuple_Pack( 1, PyFloat_FromDouble( _fall_start_pos - new_pos[2] ) );
			PyObject_CallObject( _fall_callback, args );
		}
#endif
	}
}

void PhysicsCharacterController::process_jumping()
{
	if ( _head_contact.has_contact && _capsule_top >= _head_contact.hit_pos[2] )
	{
		// If we hit the ceiling, start to fall.
		fall();
		return;
	}

	if ( !_above_ground )
	{
		// Don't jump either if we are not above a ground.
		// Emulate the original Toontown mechanisms.
		return;
	}

	float old_pos = _target_pos[2];
	_jump_time += _timestep;
	_target_pos[2] = ( _gravity * std::powf( _jump_time, 2.0f ) ) + ( _jump_speed * _jump_time ) + _jump_start_pos;

	if ( old_pos > _target_pos[2] )
	{
		fall();
	}
		
}

void PhysicsCharacterController::process_swimming()
{
	if ( _foot_contact.has_contact )
	{
		float abs_slope_dot = _foot_contact.hit_normal.dot( LVector3::up() );
		if ( _target_pos[2] - 0.1f < _foot_contact.hit_pos[2] && ( _linear_velocity[2] < 0.0f || abs_slope_dot < 1.0f ) )
		{
			_target_pos[2] = _foot_contact.hit_pos[2];
			_linear_velocity[2] = 0.0f;
		}
	}

	if ( _head_contact.has_contact && _capsule_top >= _head_contact.hit_pos[2] && _linear_velocity[2] > 0.0f )
	{
		_linear_velocity[2] = 0.0f;
	}
}

void PhysicsCharacterController::process_ground()
{
	if ( !is_on_ground() )
		fall();
	else
		_target_pos[2] = _foot_contact.hit_pos[2];
}

void PhysicsCharacterController::set_collide_mask( const BitMask32 &mask )
{
	_walk_capsule_data.capsule_np.set_collide_mask( mask );
	_crouch_capsule_data.capsule_np.set_collide_mask( mask );
	_event_sphere_np.set_collide_mask( mask );
}