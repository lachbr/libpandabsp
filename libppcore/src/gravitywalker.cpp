#include "gravitywalker.h"
#include "controlmanager.h"
#include "pp_globals.h"

#include <throw_event.h>
#include <collisionSphere.h>

static const PN_stdfloat hard_landing = -30.0;
static const PN_stdfloat soft_landing = -5.0;
static const PN_stdfloat diagonal_factor = sqrt( 2.0 ) / 2.0;
static const PN_stdfloat slide_factor = 0.75;

GravityWalker::GravityWalker( PN_stdfloat grav, PN_stdfloat standable_ground, PN_stdfloat hard_landing_force, bool legacy_lifter )
{
        _gravity = grav;
        _standable_ground = standable_ground;
        _hard_landing_force = hard_landing_force;
        _legacy_lifter = legacy_lifter;

        _may_jump = true;
}

void GravityWalker::setup_ray( BitMask32 &bitmask, PN_stdfloat floor_offset, PN_stdfloat reach )
{
        _coll_ray = new CollisionRay( 0.0, 0.0, COLL_HANDLER_RAY_START, 0.0, 0.0, -1.0 );
        PT( CollisionNode ) craynode = new CollisionNode( "GW.cRayNode" );
        craynode->add_solid( _coll_ray );
        craynode->set_from_collide_mask( bitmask );
        craynode->set_into_collide_mask( BitMask32::all_off() );
        _coll_ray_np = _avatar_np.attach_new_node( craynode );

        _lifter = new CollisionHandlerGravity();
        _lifter->set_legacy_mode( _legacy_lifter );
        _lifter->set_gravity( _gravity );
        _lifter->add_in_pattern( "enter%in" );
        _lifter->add_again_pattern( "again%in" );
        _lifter->add_out_pattern( "exit%in" );
        _lifter->set_offset( floor_offset );
        _lifter->set_reach( reach );

        _lifter->add_collider( _coll_ray_np, _avatar_np );
}

void GravityWalker::setup_wall_sphere( BitMask32 &bitmask, PN_stdfloat avatar_radius )
{
        _avatar_radius = avatar_radius;
        PT( CollisionSphere ) csphere = new CollisionSphere( 0.0, 0.0, avatar_radius, avatar_radius );
        PT( CollisionNode ) cspherenode = new CollisionNode( "GW.cWallSphereNode" );
        cspherenode->add_solid( csphere );
        cspherenode->set_from_collide_mask( bitmask );
        cspherenode->set_into_collide_mask( BitMask32::all_off() );
        _cwallsphere_np = _avatar_np.attach_new_node( cspherenode );

        if ( gw_fluid_pusher )
        {
                _pusher_f = new CollisionHandlerFluidPusher();
                _pusher_f->add_collider( _cwallsphere_np, _avatar_np );
        }
        else
        {
                _pusher = new CollisionHandlerPusher();
                _pusher->add_collider( _cwallsphere_np, _avatar_np );
        }
}

void GravityWalker::setup_event_sphere( BitMask32 &bitmask, PN_stdfloat avatar_radius )
{
        _avatar_radius = avatar_radius;
        PT( CollisionSphere ) csphere = new CollisionSphere( 0.0, 0.0, avatar_radius - 0.1, avatar_radius * 1.04 );
        csphere->set_tangible( false );
        PT( CollisionNode ) cspherenode = new CollisionNode( "GW.cEventSphereNode" );
        cspherenode->add_solid( csphere );
        cspherenode->set_from_collide_mask( bitmask );
        cspherenode->set_into_collide_mask( BitMask32::all_off() );
        _ceventsphere_np = _avatar_np.attach_new_node( cspherenode );

        _event = new CollisionHandlerEvent();
        _event->add_in_pattern( "enter%in" );
        _event->add_out_pattern( "exit%in" );
}

void GravityWalker::setup_floor_sphere( BitMask32 &bitmask, PN_stdfloat avatar_radius )
{
        _avatar_radius = avatar_radius;
        PT( CollisionSphere ) csphere = new CollisionSphere( 0.0, 0.0, avatar_radius, 0.01 );
        PT( CollisionNode ) cspherenode = new CollisionNode( "GW.cFloorSphereNode" );
        cspherenode->add_solid( csphere );
        cspherenode->set_from_collide_mask( bitmask );
        cspherenode->set_into_collide_mask( BitMask32::all_off() );
        _cfloorsphere_np = _avatar_np.attach_new_node( cspherenode );

        _pusher_floor = new CollisionHandlerPusher();
        _pusher_floor->add_collider( _cfloorsphere_np, _avatar_np );
}

void GravityWalker::set_wall_bitmask( BitMask32 &bitmask )
{
        _wall_bitmask = bitmask;
}

void GravityWalker::set_floor_bitmask( BitMask32 &bitmask )
{
        _floor_bitmask = bitmask;
}

void GravityWalker::set_event_bitmask( BitMask32 &bitmask )
{
        _event_bitmask = bitmask;
}

void GravityWalker::swap_floor_bitmask( BitMask32 &old_bm, BitMask32 &new_bm )
{
        _floor_bitmask = _floor_bitmask & ~old_bm;
        _floor_bitmask |= new_bm;

        PT( CollisionNode ) cnode = DCAST( CollisionNode, _coll_ray_np.node() );
        cnode->set_from_collide_mask( _floor_bitmask );
}

void GravityWalker::set_gravity( PN_stdfloat grav )
{
        _gravity = grav;
        _lifter->set_gravity( _gravity );
}

PN_stdfloat GravityWalker::get_gravity() const
{
        return _gravity;
}

void GravityWalker::initialize_collisions( CollisionTraverser *ctrav, NodePath &avatar_np,
                                           PN_stdfloat avatar_radius, PN_stdfloat floor_offset,
                                           PN_stdfloat reach )
{
        _avatar_np = avatar_np;
        _ctrav = ctrav;

        setup_ray( _floor_bitmask, floor_offset, reach );
        setup_wall_sphere( _wall_bitmask, avatar_radius );
        setup_event_sphere( _wall_bitmask, avatar_radius );
        if ( gw_floor_sphere )
        {
                setup_floor_sphere( _floor_bitmask, avatar_radius );
        }

        set_collisions_active( true );
}

void GravityWalker::set_tag( const string &key, const string &value )
{
        _ceventsphere_np.set_tag( key, value );
}

PN_stdfloat GravityWalker::get_airborne_height() const
{
        return _lifter->get_airborne_height();
}

void GravityWalker::set_avatar_physics_indicator( NodePath &np )
{
        _cwallsphere_np.show();
}

void GravityWalker::delete_collisions()
{
        _cwallsphere_np.remove_node();
        if ( gw_floor_sphere )
        {
                _cfloorsphere_np.remove_node();
        }
}

void GravityWalker::set_collisions_active( bool active )
{
        if ( _collisions_active != active )
        {
                _collisions_active = active;
                one_time_collide();
                if ( active )
                {
                        if ( 1 )
                        {
                                _avatar_np.set_p( 0.0 );
                                _avatar_np.set_r( 0.0 );
                        }
                        if ( gw_fluid_pusher )
                        {
                                _ctrav->add_collider( _cwallsphere_np, _pusher_f );
                        }
                        else
                        {
                                _ctrav->add_collider( _cwallsphere_np, _pusher );
                        }
                        if ( gw_floor_sphere )
                        {
                                _ctrav->add_collider( _cfloorsphere_np, _pusher_floor );
                        }
                        g_base->_shadow_trav.add_collider( _coll_ray_np, _lifter );
                        if ( gw_early_event_sphere )
                        {
                                _ctrav->add_collider( _ceventsphere_np, _event );
                        }
                        else
                        {
                                g_base->_shadow_trav.add_collider( _ceventsphere_np, _event );
                        }
                }
                else
                {
                        if ( _ctrav != (CollisionTraverser *)nullptr )
                        {
                                _ctrav->remove_collider( _cwallsphere_np );
                                if ( gw_floor_sphere )
                                {
                                        _ctrav->remove_collider( _cfloorsphere_np );
                                }
                                _ctrav->remove_collider( _ceventsphere_np );
                                _ctrav->remove_collider( _coll_ray_np );
                        }
                        g_base->_shadow_trav.remove_collider( _ceventsphere_np );
                        g_base->_shadow_trav.remove_collider( _coll_ray_np );
                }
        }
}

bool GravityWalker::get_collisions_active() const
{
        return _collisions_active;
}

void GravityWalker::place_on_floor()
{
        one_time_collide();
        _avatar_np.set_z( _avatar_np.get_z() - _lifter->get_airborne_height() );
}

void GravityWalker::one_time_collide()
{
        if ( _cwallsphere_np.is_empty() )
        {
                return;
        }

        _airborne = false;
        _may_jump = true;

        CollisionTraverser temp_ctrav( "oneTimeCollide" );
        if ( gw_fluid_pusher )
        {
                temp_ctrav.add_collider( _cwallsphere_np, _pusher_f );
        }
        else
        {
                temp_ctrav.add_collider( _cwallsphere_np, _pusher );
        }
        if ( gw_floor_sphere )
        {
                temp_ctrav.add_collider( _cfloorsphere_np, _pusher_floor );
        }
        temp_ctrav.add_collider( _coll_ray_np, _lifter );
        temp_ctrav.traverse( g_render );
}

void GravityWalker::set_may_jump()
{
        _may_jump = true;
}

AsyncTask::DoneStatus GravityWalker::jump_task( GenericAsyncTask *task, void *data )
{
        ( (GravityWalker *)data )->set_may_jump();
        return AsyncTask::DS_done;
}

void GravityWalker::start_jump_delay( PN_stdfloat delay )
{
        if ( _jump_delay_task != (GenericAsyncTask *)nullptr )
        {
                _jump_delay_task->remove();
        }
        _may_jump = false;
        _jump_delay_task = new GenericAsyncTask( "GW.jumpDelay", jump_task, this );
        _jump_delay_task->set_delay( delay );
        g_task_mgr->add( _jump_delay_task );
}

void GravityWalker::add_blast_force( LVecBase3f &vector )
{
        _lifter->add_velocity( vector.length() );
}

string GravityWalker::get_hard_land_event() const
{
        return "jumpHardLand";
}

string GravityWalker::get_land_event() const
{
        return "jumpLand";
}

string GravityWalker::get_jump_start_event() const
{
        return "jumpStart";
}

string GravityWalker::get_moving_event() const
{
        return "moving";
}

void GravityWalker::handle_avatar_controls()
{
        bool run = g_input_state.is_set( "run" );
        bool fwd = g_input_state.is_set( "forward" );
        bool rev = g_input_state.is_set( "reverse" );
        bool tl = g_input_state.is_set( "turnLeft" );
        bool tr = g_input_state.is_set( "turnRight" );
        bool sl = g_input_state.is_set( "slideLeft" );
        bool sr = g_input_state.is_set( "slideRight" );
        bool jump = g_input_state.is_set( "jump" );

        if ( fwd )
        {
                _speed = _avatar_control_forward_speed;
        }
        else if ( rev )
        {
                _speed = -_avatar_control_reverse_speed;
        }
        else
        {
                _speed = 0.0;
        }

        if ( rev && sl )
        {
                _slide_speed = -_avatar_control_reverse_speed * slide_factor;
        }
        else if ( rev && sr )
        {
                _slide_speed = _avatar_control_reverse_speed * slide_factor;
        }
        else if ( sl )
        {
                _slide_speed = -_avatar_control_forward_speed * slide_factor;
        }
        else if ( sr )
        {
                _slide_speed = _avatar_control_forward_speed * slide_factor;
        }
        else
        {
                _slide_speed = 0.0;
        }

        if ( !( sl || sr ) )
        {
                if ( tl )
                {
                        _rotation_speed = _avatar_control_rotate_speed;
                }
                else if ( tr )
                {
                        _rotation_speed = -_avatar_control_reverse_speed;
                }
                else
                {
                        _rotation_speed = 0.0;
                }
        }
        else
        {
                _rotation_speed = 0.0;
        }

        if ( _speed != 0.0 && _slide_speed != 0.0 )
        {
                _speed *= diagonal_factor;
                _slide_speed *= diagonal_factor;
        }

        if ( _need_to_delta_pos )
        {
                set_prior_parent_vector();
                _need_to_delta_pos = true;
        }

        if ( _lifter->is_on_ground() )
        {
                if ( _airborne )
                {
                        _airborne = false;
                        PN_stdfloat impact = _lifter->get_impact_velocity();
                        if ( impact < hard_landing )
                        {
                                throw_event( get_hard_land_event() );
                                start_jump_delay( 0.3 );
                        }
                        else
                        {
                                throw_event( get_land_event() );
                                if ( impact < soft_landing )
                                {
                                        start_jump_delay( 0.2 );
                                }
                        }
                }

                _prior_parent = LVector3::zero();

                if ( jump && _may_jump )
                {
                        _lifter->add_velocity( _avatar_control_jump_force );
                        throw_event( get_jump_start_event() );
                        _airborne = true;
                }
        }
        else
        {
                _airborne = true;
        }

        _old_pos_delta = _avatar_np.get_pos_delta();
        _old_dt = g_global_clock->get_dt();
        PN_stdfloat dt = _old_dt;

        bool moving = _speed || _slide_speed || _rotation_speed || ( _prior_parent != LVector3::zero() );
        if ( moving )
        {
                PN_stdfloat distance = dt * _speed;
                PN_stdfloat sl_dist = dt * _slide_speed;
                PN_stdfloat rot_dist = dt * _rotation_speed;

                if ( distance || sl_dist || ( _prior_parent != LVector3::zero() ) )
                {
                        LMatrix3 rot_mat = LMatrix3::rotate_mat_normaxis( _avatar_np.get_h(), LVector3::up() );
                        LVector3 forward;
                        LVector3 contact;

                        if ( _airborne )
                        {
                                forward = LVector3::forward();
                        }
                        else
                        {
                                contact = _lifter->get_contact_normal();
                                forward = contact.cross( LVector3::right() );
                                forward.normalize();
                        }

                        _vel = LVector3( forward * distance );

                        if ( sl_dist )
                        {
                                LVector3 right;
                                if ( _airborne )
                                {
                                        right = LVector3::right();
                                }
                                else
                                {
                                        right = forward.cross( contact );
                                        right.normalize();
                                }
                                _vel = LVector3( _vel + ( right * sl_dist ) );
                        }
                        _vel = LVector3( rot_mat.xform( _vel ) );
                        LVector3 step = _vel + ( _prior_parent * dt );
                        _avatar_np.set_fluid_pos( _avatar_np.get_pos() + step );
                }
                _avatar_np.set_h( _avatar_np.get_h() + rot_dist );
        }
        else
        {
                _vel.set( 0.0, 0.0, 0.0 );
        }

        if ( moving || jump )
        {
                throw_event( get_moving_event() );
        }
}

AsyncTask::DoneStatus GravityWalker::av_ctrl_task( GenericAsyncTask *task, void *data )
{
        ( (GravityWalker *)data )->handle_avatar_controls();
        return AsyncTask::DS_cont;
}

void GravityWalker::do_delta_pos()
{
        _need_to_delta_pos = true;
}

void GravityWalker::set_prior_parent_vector()
{
        LVector3 vel;
        if ( _old_dt == 0 )
        {
                vel = LVector3::zero();
        }
        else
        {
                vel = _old_pos_delta * ( 1.0 / _old_dt );
        }

        _prior_parent = vel;
}

void GravityWalker::reset()
{
        _lifter->set_velocity( 0.0 );
        _prior_parent = LVector3::zero();
}

const LVector3 &GravityWalker::get_velocity() const
{
        return _vel;
}

void GravityWalker::enable_avatar_controls()
{
        if ( _controls_task != (GenericAsyncTask *)nullptr )
        {
                _controls_task->remove();
        }

        string task_name = "AvatarControls";
        _controls_task = new GenericAsyncTask( task_name, av_ctrl_task, this );

        _airborne = false;
        _may_jump = true;

        g_task_mgr->add( _controls_task );
}

void GravityWalker::disable_avatar_controls()
{
        if ( _controls_task != (GenericAsyncTask *)nullptr )
        {
                _controls_task->remove();
        }
        if ( _indicator_task != (GenericAsyncTask *)nullptr )
        {
                _indicator_task->remove();
        }
        if ( _jump_delay_task != (GenericAsyncTask *)nullptr )
        {
                _jump_delay_task->remove();
        }
}

void GravityWalker::flush_event_handlers()
{
        if ( _ctrav != (CollisionTraverser *)nullptr )
        {
                if ( gw_fluid_pusher )
                {
                        _pusher_f->flush();
                }
                else
                {
                        _pusher->flush();
                }
                if ( gw_floor_sphere )
                {
                        _pusher_floor->flush();
                }
                _event->flush();
        }
        _lifter->flush();
}

void GravityWalker::set_collision_ray_height( PN_stdfloat height )
{
        _coll_ray->set_origin( 0.0, 0.0, height );
}