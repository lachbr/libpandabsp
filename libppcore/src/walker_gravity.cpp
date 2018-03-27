#include "walker_gravity.h"
#include "controlmanager.h"
#include "pp_globals.h"

#include <throw_event.h>
#include <collisionSphere.h>

GravityWalker::GravityWalker( PN_stdfloat grav, PN_stdfloat standable_ground,
                              PN_stdfloat hard_landing_force, bool legacy_lifter ) :
        BaseWalker( grav, standable_ground, hard_landing_force ),
        _legacy_lifter( legacy_lifter )
{
        if ( gw_fluid_pusher )
                _pusher = new CollisionHandlerFluidPusher;

        _need_to_delta_pos = false;

        _may_jump = true;
}

void GravityWalker::setup_ray( const BitMask32 &bitmask, PN_stdfloat floor_offset, PN_stdfloat reach )
{
        BaseWalker::setup_ray( bitmask );

        _lifter = new CollisionHandlerGravity;
        _lifter->set_legacy_mode( _legacy_lifter );
        _lifter->set_gravity( _gravity );
        _lifter->add_in_pattern( "enter%in" );
        _lifter->add_again_pattern( "again%in" );
        _lifter->add_out_pattern( "exit%in" );
        _lifter->set_offset( floor_offset );
        _lifter->set_reach( reach );

        _lifter->add_collider( _coll_ray_np, _avatar_np );
}

void GravityWalker::set_gravity( PN_stdfloat grav )
{
        BaseWalker::set_gravity( grav );
        _lifter->set_gravity( grav );
}

void GravityWalker::initialize_collisions( CollisionTraverser *ctrav, const NodePath &avatar_np,
                                           PN_stdfloat avatar_radius, PN_stdfloat floor_offset,
                                           PN_stdfloat reach )
{
        BaseWalker::initialize_collisions( ctrav, avatar_np, avatar_radius,
                                           floor_offset, reach );

        setup_ray( _floor_bitmask, floor_offset, reach );

        set_collisions_active( true );
}

PN_stdfloat GravityWalker::get_airborne_height() const
{
        return _lifter->get_airborne_height();
}

void GravityWalker::delete_collisions()
{
        BaseWalker::delete_collisions();
}

void GravityWalker::set_collisions_active( bool active )
{
        if ( _collisions_active != active )
        {
                if ( active )
                {
                        g_base->_shadow_trav.add_collider( _coll_ray_np, _lifter );
                }
        }

        BaseWalker::set_collisions_active( active );
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
        temp_ctrav.add_collider( _cwallsphere_np, _pusher );
        temp_ctrav.add_collider( _cfloorsphere_np, _pusher_floor );
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
        if ( _jump_delay_task != nullptr )
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

void GravityWalker::handle_avatar_controls()
{
        InputData idat = get_input_data();
        update_speeds( idat );

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
                        throw_event( get_land_event(), EventParameter( impact ) );
                }

                _prior_parent = LVector3::zero();

                if ( idat.jump && _may_jump )
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
        double dt = _old_dt;

        bool moving = _speeds != LVector3::zero() || ( _prior_parent != LVector3::zero() );
        if ( moving )
        {
                PN_stdfloat distance = dt * _speeds.get_x();
                PN_stdfloat sl_dist = dt * _speeds.get_y();
                PN_stdfloat rot_dist = dt * _speeds.get_z();

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

        if ( moving || idat.jump )
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

void GravityWalker::disable_avatar_controls()
{
        BaseWalker::disable_avatar_controls();

        if ( _indicator_task != nullptr )
        {
                _indicator_task->remove();
        }
        if ( _jump_delay_task != nullptr )
        {
                _jump_delay_task->remove();
        }
}

void GravityWalker::flush_event_handlers()
{
        BaseWalker::flush_event_handlers();
        if ( gw_floor_sphere )
        {
                _pusher_floor->flush();
        }
        _lifter->flush();
}