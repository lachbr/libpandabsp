#include "walker_physics.h"
#include "pp_globals.h"
#include "global_physicsmanager.h"

#include <physicsManager.h>
#include <forceNode.h>
#include <linearEulerIntegrator.h>
#include <physicsCollisionHandler.h>
#include <throw_event.h>

PhysicsWalker::PhysicsWalker( PN_stdfloat gravity, PN_stdfloat standable_ground,
                              PN_stdfloat hard_landing_force ) :
        BaseWalker( gravity, standable_ground, hard_landing_force ),
        _actor( nullptr ),
        _cray_queue( nullptr )
{
        _pusher = new PhysicsCollisionHandler;
}

PN_stdfloat PhysicsWalker::get_airborne_height() const
{
        PN_stdfloat height = 0.0;
        if ( _cray_queue->get_num_entries() != 0 )
        {
                _cray_queue->sort_entries();
                LPoint3 flr_point = _cray_queue->get_entry( 0 )->get_surface_point( g_render );
                height = flr_point.get_z();
        }
        return height;
}

void PhysicsWalker::setup_ray( const BitMask32 &bitmask )
{
        BaseWalker::setup_ray( bitmask );

        _cray_queue = new CollisionHandlerQueue;
        g_base->_shadow_trav.add_collider( _coll_ray_np, _cray_queue );
}

void PhysicsWalker::set_avatar( const NodePath &avatar )
{
        BaseWalker::set_avatar( avatar );
        if ( !avatar.is_empty() )
                setup_physics();
}

void PhysicsWalker::initialize_collisions( CollisionTraverser *ctrav, const NodePath &avatar_np,
                                           PN_stdfloat avatar_radius, PN_stdfloat flr_offset,
                                           PN_stdfloat reach )
{
        _avatar_np = avatar_np;

        setup_physics();
        BaseWalker::initialize_collisions( ctrav, _avatar_np, avatar_radius, flr_offset, reach );
        setup_ray( _floor_bitmask );

        DCAST( CollisionNode, _cwallsphere_np.node() )->set_from_collide_mask( _wall_bitmask | _floor_bitmask );

        set_collisions_active( true );
}

NodePath PhysicsWalker::get_physics_actor() const
{
        return _physics_actor_np;
}

void PhysicsWalker::setup_physics()
{
        _actor = new ActorNode( "physicsActor" );
        _actor->get_physics_object()->set_oriented( true );
        NodePath physics_actor( _actor );
        _avatar_np.reparent_to( physics_actor );
        _physics_actor_np = physics_actor;
        _avatar_np.operator=( _physics_actor_np );
        GlobalPhysicsManager::physics_mgr.attach_physical_node( _actor );
}

void PhysicsWalker::set_collisions_active( bool active )
{
        if ( _collisions_active != active )
        {
                if ( active )
                {
                        g_base->_shadow_trav.add_collider( _coll_ray_np, _cray_queue );
                }
        }

        BaseWalker::set_collisions_active( active );
}

void PhysicsWalker::one_time_collide()
{
        CollisionTraverser temp_trav( "oneTimeCollide" );
        temp_trav.add_collider( _coll_ray_np, _cray_queue );
        temp_trav.traverse( g_render );
}

void PhysicsWalker::handle_avatar_controls()
{
        PhysicsObject *phys_obj = _actor->get_physics_object();
        LVector3 contact = _actor->get_contact_vector();

        InputData idat = get_input_data();
        update_speeds( idat );

        double dt = ClockObject::get_global_clock()->get_dt();
        
        PN_stdfloat ground_height = get_airborne_height();
        // The height that classifies us as airborne (just a tiny bit off the ground level).
        PN_stdfloat airborne_height = ground_height + 0.05;

        if ( phys_obj->get_position().get_z() >= airborne_height )
        {
                _airborne = true;
        }
        else
        {
                if ( _airborne && phys_obj->get_position().get_z() < airborne_height )
                {                        
                        // We've landed, find out the landing force and send out an event.
                        PN_stdfloat landing_force = contact.length();
                        throw_event( get_land_event(), EventParameter( landing_force ) );
                        _airborne = false;
                }
                else if ( idat.jump )
                {
                        throw_event( get_jump_start_event() );
                        // Jump straight up!
                        phys_obj->add_impulse( LVector3::up() * _avatar_control_jump_force );
                        _airborne = true;
                }
                else
                {
                        _airborne = false;
                }
        }

        if ( _speeds != LVector3::zero() )
        {
                PN_stdfloat distance = dt * _speeds.get_x();
                PN_stdfloat sl_dist = dt * _speeds.get_y();
                PN_stdfloat rot_dist = dt * _speeds.get_z();

                // Update position
                _vel = (LVector3::forward() * distance) + (LVector3::right() * sl_dist);

                LMatrix3 rot_mat = LMatrix3::rotate_mat_normaxis( _avatar_np.get_h(), LVector3::up() );
                _vel = rot_mat.xform( _vel );
                phys_obj->set_position( phys_obj->get_position() + _vel );

                // Update heading
                LOrientation orient = phys_obj->get_orientation();
                LRotation rot;
                rot.set_hpr( LVector3( rot_dist, 0, 0 ) );
                phys_obj->set_orientation( orient * rot );

                throw_event( get_moving_event() );
                _moving = true;
        }
        else
        {
                _vel.set( 0.0, 0.0, 0.0 );
                _moving = false;
        }
}

void PhysicsWalker::reset()
{
        _actor->get_physics_object()->reset_position( _avatar_np.get_pos() );
        _actor->set_contact_vector( LVector3::zero() );
}

LVector3 PhysicsWalker::get_velocity() const
{
        // The velocity is the sum of the velocity applied from actual physics
        // and the velocity of our movement controls.
        return ( _actor->get_physics_object()->get_velocity() * 0.005 ) + _vel;
}

LVector3 PhysicsWalker::get_control_velocity() const
{
        return _vel;
}