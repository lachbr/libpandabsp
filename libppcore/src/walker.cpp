#include "walker.h"
#include "pp_globals.h"

#include <collisionSphere.h>

static const PN_stdfloat hard_landing = -30.0;
static const PN_stdfloat soft_landing = -5.0;
static const PN_stdfloat diagonal_factor = sqrt( 2.0 ) / 2.0;
static const PN_stdfloat slide_factor = 0.75;

BaseWalker::BaseWalker()
{
}

BaseWalker::BaseWalker( PN_stdfloat gravity, PN_stdfloat standable_ground,
                        PN_stdfloat hard_landing_force ) :
        _gravity( gravity ),
        _standable_ground( standable_ground ),
        _hard_landing_force( hard_landing_force ),
        _speeds( 0 ),
        _vel( 0 ),
        _avatar_control_forward_speed( 0 ),
        _avatar_control_jump_force( 0 ),
        _avatar_control_reverse_speed( 0 ),
        _avatar_control_rotate_speed( 0 ),
        _airborne( false ),
        _may_jump( false ),
        _moving( false ),
        _collisions_active( false ),
        _coll_ray( nullptr ),
        _ctrav( nullptr ),
        _tick_task( new GenericAsyncTask( "walkerTickTask", tick_task, this ) ),
        _avatar_radius( 0.0 ),
        _friction( 1.0 ),
        _last_speeds( 0 ),
        _pusher( new CollisionHandlerPusher ),
        _event( new CollisionHandlerEvent ),
        _pusher_floor( new CollisionHandlerPusher )
{
}

void BaseWalker::setup_floor_sphere( const BitMask32 &bitmask, PN_stdfloat avatar_radius )
{
        _avatar_radius = avatar_radius;
        PT( CollisionSphere ) csphere = new CollisionSphere( 0.0, 0.0, avatar_radius, 0.01 );
        PT( CollisionNode ) cspherenode = new CollisionNode( "cFloorSphereNode" );
        cspherenode->add_solid( csphere );
        cspherenode->set_from_collide_mask( bitmask );
        cspherenode->set_into_collide_mask( BitMask32::all_off() );
        _cfloorsphere_np = _avatar_np.attach_new_node( cspherenode );

        _pusher_floor->add_collider( _cfloorsphere_np, _avatar_np );
}

void BaseWalker::enable_avatar_controls()
{
        _airborne = false;
        _may_jump = true;
        AsyncTaskManager::get_global_ptr()->add( _tick_task );
}

void BaseWalker::disable_avatar_controls()
{
        _tick_task->remove();
}

LVector3 BaseWalker::get_velocity() const
{
        return _vel;
}

BaseWalker::InputData BaseWalker::get_input_data() const
{
        InputData idat;
        idat.fwd = g_input_state.is_set( "forward" );
        idat.rev = g_input_state.is_set( "reverse" );
        idat.tl = g_input_state.is_set( "turnLeft" );
        idat.tr = g_input_state.is_set( "turnRight" );
        idat.sl = g_input_state.is_set( "slideLeft" );
        idat.sr = g_input_state.is_set( "slideRight" );
        idat.jump = g_input_state.is_set( "jump" );

        return idat;
}


void BaseWalker::set_friction( PN_stdfloat friction )
{
        _friction = friction;
}

PN_stdfloat BaseWalker::get_friction() const
{
        return _friction;
}

void BaseWalker::update_speeds( const InputData &data, bool apply_friction )
{
        if ( data.fwd )
                _speeds.set_x( _avatar_control_forward_speed );
        else if ( data.rev )
                _speeds.set_x( -_avatar_control_reverse_speed );
        else
                _speeds.set_x( 0.0 );

        if ( data.rev && data.sl )
                _speeds.set_y( -_avatar_control_reverse_speed * slide_factor );
        else if ( data.rev && data.sr )
                _speeds.set_y( _avatar_control_reverse_speed * slide_factor );
        else if ( data.sl )
                _speeds.set_y( -_avatar_control_forward_speed * slide_factor );
        else if ( data.sr )
                _speeds.set_y( _avatar_control_forward_speed * slide_factor );
        else
                _speeds.set_y( 0.0 );

        if ( !( data.sl || data.sr ) )
        {
                if ( data.tl )
                        _speeds.set_z( _avatar_control_rotate_speed );
                else if ( data.tr )
                        _speeds.set_z( -_avatar_control_reverse_speed );
                else
                        _speeds.set_z( 0.0 );
        }
        else
                _speeds.set_z( 0.0 );

        if ( _speeds.get_x() != 0.0 && _speeds.get_y() != 0.0 )
        {
                _speeds.set_x( _speeds.get_x() * diagonal_factor );
                _speeds.set_y( _speeds.get_y() * diagonal_factor );
        }

        if ( apply_friction )
        {
                double dt = g_global_clock->get_dt();
                double friction = 1 - pow( 1 - get_friction(), dt * 30.0 );
                _last_speeds.set_x( _speeds.get_x() * friction + _last_speeds.get_x() * ( 1 - friction ) );
                _last_speeds.set_y( _speeds.get_y() * friction + _last_speeds.get_y() * ( 1 - friction ) );
                _last_speeds.set_z( _speeds.get_z() * friction + _last_speeds.get_z() * ( 1 - friction ) );

                _speeds = _last_speeds;

                if ( abs( _speeds.get_x() ) < 0.1 )
                        _speeds.set_x( 0.0 );
                if ( abs( _speeds.get_y() ) < 0.1 )
                        _speeds.set_y( 0.0 );
                if ( abs( _speeds.get_z() ) < 0.1 )
                        _speeds.set_z( 0.0 );
        }
}

void BaseWalker::cleanup()
{
}

void BaseWalker::flush_event_handlers()
{
        _pusher->flush();
        _pusher_floor->flush();
        _event->flush();
}

void BaseWalker::set_collision_ray_height( PN_stdfloat height )
{
        _coll_ray->set_origin( 0.0, 0.0, height );
}

void BaseWalker::set_avatar( const NodePath &avatar )
{
        _avatar_np = avatar;
}

void BaseWalker::set_gravity( PN_stdfloat grav )
{
        _gravity = grav;
}

PN_stdfloat BaseWalker::get_gravity() const
{
        return _gravity;
}

void BaseWalker::initialize_collisions( CollisionTraverser *ctrav, const NodePath &avatar_np,
                                        PN_stdfloat avatar_radius, PN_stdfloat floor_offset,
                                        PN_stdfloat reach )
{
        _avatar_np = avatar_np;
        _ctrav = ctrav;

        setup_wall_sphere( _wall_bitmask, avatar_radius );
        setup_event_sphere( _wall_bitmask, avatar_radius );
        setup_floor_sphere( _floor_bitmask, avatar_radius );
}

void BaseWalker::set_walk_speed( PN_stdfloat forward, PN_stdfloat jump, PN_stdfloat reverse, PN_stdfloat rotate )
{
        _avatar_control_forward_speed = forward;
        _avatar_control_jump_force = jump;
        _avatar_control_reverse_speed = reverse;
        _avatar_control_rotate_speed = rotate;
}

LVector3 BaseWalker::get_speeds() const
{
        return _speeds;
}

bool BaseWalker::get_is_airborne() const
{
        return _airborne;
}

void BaseWalker::set_tag( const string &key, const string &value )
{
        _ceventsphere_np.set_tag( key, value );
}

void BaseWalker::place_on_floor()
{
        one_time_collide();
        _avatar_np.set_z( _avatar_np.get_z() - get_airborne_height() );
}

PN_stdfloat BaseWalker::get_airborne_height() const
{
        return 0.0;
}

void BaseWalker::setup_ray( const BitMask32 &bitmask )
{
        _coll_ray = new CollisionRay( 0.0, 0.0, 4000.0, 0.0, 0.0, -1.0 );
        PT( CollisionNode ) craynode = new CollisionNode( "cRayNode" );
        craynode->add_solid( _coll_ray );
        craynode->set_from_collide_mask( bitmask );
        craynode->set_into_collide_mask( BitMask32::all_off() );
        _coll_ray_np = _avatar_np.attach_new_node( craynode );
}

void BaseWalker::setup_wall_sphere( const BitMask32 &bitmask, PN_stdfloat avatar_radius )
{
        _avatar_radius = avatar_radius;
        PT( CollisionSphere ) csphere = new CollisionSphere( 0.0, 0.0, avatar_radius, avatar_radius );
        PT( CollisionNode ) cspherenode = new CollisionNode( "cWallSphereNode" );
        cspherenode->add_solid( csphere );
        cspherenode->set_from_collide_mask( bitmask );
        cspherenode->set_into_collide_mask( BitMask32::all_off() );
        _cwallsphere_np = _avatar_np.attach_new_node( cspherenode );

        _pusher->set_in_pattern( "enter%in" );
        _pusher->set_out_pattern( "exit%in" );
        _pusher->add_collider( _cwallsphere_np, _avatar_np );
}

void BaseWalker::setup_event_sphere( const BitMask32 &bitmask, PN_stdfloat avatar_radius )
{
        PT( CollisionSphere ) csphere = new CollisionSphere( 0.0, 0.0, avatar_radius - 0.1, avatar_radius * 1.04 );
        csphere->set_tangible( false );
        PT( CollisionNode ) cspherenode = new CollisionNode( "cEventSphereNode" );
        cspherenode->add_solid( csphere );
        cspherenode->set_from_collide_mask( bitmask );
        cspherenode->set_into_collide_mask( BitMask32::all_off() );
        _ceventsphere_np = _avatar_np.attach_new_node( cspherenode );

        _event->add_in_pattern( "enter%in" );
        _event->add_out_pattern( "exit%in" );
}

void BaseWalker::swap_floor_bitmask( BitMask32 &old_bm, BitMask32 &new_bm )
{
        _floor_bitmask = _floor_bitmask & ~old_bm;
        _floor_bitmask |= new_bm;

        PT( CollisionNode ) cnode = DCAST( CollisionNode, _coll_ray_np.node() );
        cnode->set_from_collide_mask( _floor_bitmask );
}

void BaseWalker::set_wall_bitmask( const BitMask32 &bitmask )
{
        _wall_bitmask = bitmask;
}

void BaseWalker::set_floor_bitmask( const BitMask32 &bitmask )
{
        _floor_bitmask = bitmask;
}

void BaseWalker::set_event_bitmask( const BitMask32 &bitmask )
{
        _event_bitmask = bitmask;
}

void BaseWalker::delete_collisions()
{
        _ceventsphere_np.remove_node();
        _cwallsphere_np.remove_node();
        _coll_ray_np.remove_node();
        _cfloorsphere_np.remove_node();
}

void BaseWalker::set_collisions_active( bool active )
{
        if ( _collisions_active != active )
        {
                _collisions_active = active;
                one_time_collide();
                if ( active )
                {
                        _avatar_np.set_p( 0.0 );
                        _avatar_np.set_r( 0.0 );
                        _ctrav->add_collider( _cwallsphere_np, _pusher );
                        if ( gw_early_event_sphere )
                        {
                                _ctrav->add_collider( _ceventsphere_np, _event );
                        }
                        else
                        {
                                g_base->_shadow_trav.add_collider( _ceventsphere_np, _event );
                        }
                        _ctrav->add_collider( _cfloorsphere_np, _pusher_floor );
                }
                else
                {
                        if ( _ctrav != nullptr )
                        {
                                _ctrav->remove_collider( _cwallsphere_np );
                                _ctrav->remove_collider( _ceventsphere_np );
                                _ctrav->remove_collider( _coll_ray_np );
                                _ctrav->remove_collider( _cfloorsphere_np );
                        }
                        g_base->_shadow_trav.remove_collider( _ceventsphere_np );
                        g_base->_shadow_trav.remove_collider( _coll_ray_np );
                }
        }
}

bool BaseWalker::get_collisions_active() const
{
        return _collisions_active;
}

void BaseWalker::handle_avatar_controls()
{
}

AsyncTask::DoneStatus BaseWalker::tick_task( GenericAsyncTask *task, void *data )
{
        BaseWalker *self = (BaseWalker *)data;
        self->handle_avatar_controls();

        return AsyncTask::DS_cont;
}

string BaseWalker::get_hard_land_event() const
{
        return "jumpHardLand";
}

string BaseWalker::get_land_event() const
{
        return "jumpLand";
}

string BaseWalker::get_jump_start_event() const
{
        return "jumpStart";
}

string BaseWalker::get_moving_event() const
{
        return "moving";
}

void BaseWalker::reset()
{
}

void BaseWalker::one_time_collide()
{
}