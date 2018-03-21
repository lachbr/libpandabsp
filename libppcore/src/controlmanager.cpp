#include "controlmanager.h"
#include "pp_globals.h"

ConfigVariableBool ControlManager::_want_wasd( "want-WASD", false );

ControlManager::
ControlManager( bool enable_now, bool pass_messages_through ) :
        _current_controls( NULL )
{
        _pass_messages_through = pass_messages_through;
        _wasd_turn = false;

        if ( enable_now )
        {
                enable();
        }

        if ( pass_messages_through )
        {
                _inputs.push_back( g_input_state.define_input( "forward", "arrow_up", "arrow_up-up" ) );
                _inputs.push_back( g_input_state.define_input( "reverse", "arrow_down", "arrow_down-up" ) );
                _inputs.push_back( g_input_state.define_input( "turnLeft", "arrow_left", "arrow_left-up" ) );
                _inputs.push_back( g_input_state.define_input( "turnRight", "arrow_right", "arrow_right-up" ) );
        }
}

bool ControlManager::
control_sys_exists( const string &name )
{
        for ( pmap<string, ControlSystemBase>::const_iterator itr = _controls.begin(); itr != _controls.end(); ++itr )
        {
                if ( name.compare( itr->first ) == 0 )
                {
                        return true;
                }
        }

        return false;
}

void ControlManager::
add( ControlSystemBase &controls, const string &name )
{
        if ( control_sys_exists( name ) )
        {
                ControlSystemBase old_ctrls = _controls[name];
                old_ctrls.disable_avatar_controls();
                old_ctrls.set_collisions_active( false );
                old_ctrls.cleanup();
        }
        controls.disable_avatar_controls();
        controls.set_collisions_active( true );
        _controls[name] = controls;
}

// Will crash if it doesn't exist!
ControlSystemBase &ControlManager::
get( const string &name )
{
        return _controls[name];
}

void ControlManager::
remove( const string &name )
{
        if ( control_sys_exists( name ) )
        {
                ControlSystemBase ctrls = _controls[name];
                ctrls.disable_avatar_controls();
                ctrls.set_collisions_active( false );
        }
}

void ControlManager::
use( const string &name, NodePath &avatar )
{
        if ( control_sys_exists( name ) )
        {
                ControlSystemBase ctrls = _controls[name];
                _current_controls->disable_avatar_controls();
                _current_controls->set_collisions_active( false );
                _current_controls->set_avatar( NodePath() );
                _current_controls = &ctrls;
                _current_controls->set_avatar( avatar );
                _current_controls->set_collisions_active( true );
                if ( _enabled )
                {
                        _current_controls->enable_avatar_controls();
                }
        }
}

void ControlManager::
set_speeds( PN_stdfloat fwd, PN_stdfloat jump, PN_stdfloat rev, PN_stdfloat rot )
{
        for ( pmap<string, ControlSystemBase>::const_iterator itr = _controls.begin(); itr != _controls.end(); ++itr )
        {
                ( ( ControlSystemBase )itr->second ).set_walk_speed( fwd, jump, rev, rot );
        }
}

void ControlManager::
cleanup()
{
        for ( pmap<string, ControlSystemBase>::const_iterator itr = _controls.begin(); itr != _controls.end(); ++itr )
        {
                remove( itr->first );
        }
        _controls.clear();

        for ( size_t i = 0; i < _inputs.size(); i++ )
        {
                g_input_state.undefine_input( _inputs[i] );
        }

        _inputs.clear();
}

ControlSystemBase::SpeedData ControlManager::
get_speeds() const
{
        return _current_controls->get_speeds();
}

bool ControlManager::
get_is_airborne() const
{
        return _current_controls->get_is_airborne();
}

void ControlManager::
set_tag( const string &key, const string &value )
{
        for ( pmap<string, ControlSystemBase>::const_iterator itr = _controls.begin(); itr != _controls.end(); ++itr )
        {
                ( ( ControlSystemBase )itr->second ).set_tag( key, value );
        }
}

void ControlManager::
delete_collisions()
{
        for ( pmap<string, ControlSystemBase>::const_iterator itr = _controls.begin(); itr != _controls.end(); ++itr )
        {
                ( ( ControlSystemBase )itr->second ).delete_collisions();
        }
}

void ControlManager::
collisions_on()
{
        for ( pmap<string, ControlSystemBase>::const_iterator itr = _controls.begin(); itr != _controls.end(); ++itr )
        {
                ( ( ControlSystemBase )itr->second ).set_collisions_active( true );
        }
}

void ControlManager::
collisions_off()
{
        for ( pmap<string, ControlSystemBase>::const_iterator itr = _controls.begin(); itr != _controls.end(); ++itr )
        {
                ( ( ControlSystemBase )itr->second ).set_collisions_active( false );
        }
}

void ControlManager::
place_on_floor()
{
        for ( pmap<string, ControlSystemBase>::const_iterator itr = _controls.begin(); itr != _controls.end(); ++itr )
        {
                ( ( ControlSystemBase )itr->second ).place_on_floor();
        }
}

void ControlManager::
enable()
{
        if ( _enabled )
        {
                return;
        }

        _enabled = true;



        if ( _want_wasd )
        {
                //_inputs.push_back(input_state.define_input("turnLeft", "arrow_left", "arrow_left-up"));
                //_ists.push_back(input_state.watch("turnLeft", "mouse-look_left", "mouse-look_left-done"));
                //_ists.push_back(input_state.watch("turnLeft", "force-turnLeft", "force-turnLeft-stop"));

                //input_state.undefine_input("forward");
                //input_state.undefine_input("reverse");

                _inputs.push_back( g_input_state.define_input( "forward", "w", "w-up" ) );
                _inputs.push_back( g_input_state.define_input( "reverse", "s", "s-up" ) );

                _inputs.push_back( g_input_state.define_input( "slideLeft", "a", "a-up" ) );
                _inputs.push_back( g_input_state.define_input( "slideRight", "d", "d-up" ) );

                //set_wasd_turn(true);

        }
        else
        {

                _inputs.push_back( g_input_state.define_input( "forward", "arrow_up", "arrow_up-up" ) );
                //_ists.push_back(input_state.watch("forward", "force-forward", "force-forward-stop"));

                _inputs.push_back( g_input_state.define_input( "reverse", "arrow_down", "arrow_down-up" ) );
                //_ists.push_back(input_state.watch_with_modifiers("reverse", "mouse4", false, IS_arrowkeys));

                _inputs.push_back( g_input_state.define_input( "turnLeft", "arrow_left", "arrow_left-up" ) );
                //_ists.push_back(input_state.watch("turnLeft", "mouse-look_left", "mouse-look_left-done"));
                //_ists.push_back(input_state.watch("turnLeft", "force-turnLeft", "force-turnLeft-stop"));

                _inputs.push_back( g_input_state.define_input( "turnRight", "arrow_right", "arrow_right-up" ) );
                //_ists.push_back(input_state.watch("turnRight", "mouse-look_right", "mouse-look_right-done"));
                //_ists.push_back(input_state.watch("turnRight", "force-turnRight", "force-turnRight-stop"));
        }

        if ( _want_wasd )
        {
                _inputs.push_back( g_input_state.define_input( "jump", "space", "space-up" ) );
        }
        else
        {
                _inputs.push_back( g_input_state.define_input( "jump", "control", "control-up" ) );
        }
}

void ControlManager::
disable()
{
        _enabled = false;

        for ( size_t i = 0; i < _inputs.size(); i++ )
        {
                g_input_state.undefine_input( _inputs[i] );
        }
        _inputs.clear();

        if ( _current_controls != NULL )
        {
                _current_controls->disable_avatar_controls();
        }


        if ( _pass_messages_through )
        {
                _inputs.push_back( g_input_state.define_input( "forward", "arrow_up", "arrow_up-up" ) );
                _inputs.push_back( g_input_state.define_input( "reverse", "arrow_down", "arrow_down-up" ) );
                _inputs.push_back( g_input_state.define_input( "turnLeft", "arrow_left", "arrow_left-up" ) );
                _inputs.push_back( g_input_state.define_input( "turnRight", "arrow_right", "arrow_right-up" ) );
        }
}

void ControlManager::
stop()
{
        disable();
        _current_controls->set_collisions_active( 0 );
        _current_controls->set_avatar( NodePath() );
}

void ControlManager::
disable_avatar_jump()
{
        g_input_state.force_input( "jump" );
}

void ControlManager::
enable_avatar_jump()
{
        g_input_state.unforce_input( "jump" );
}

void ControlManager::
set_wasd_turn( bool turn )
{
        _wasd_turn = turn;

        if ( !_enabled )
        {
                return;
        }

        bool turnLeftWASDSet = g_input_state.is_set( "turnLeft" );
        bool turnRightWASDSet = g_input_state.is_set( "turnRight" );
        bool slideLeftWASDSet = g_input_state.is_set( "slideLeft" );
        bool slideRightWASDSet = g_input_state.is_set( "slideRight" );

        if ( turn )
        {
                _inputs.push_back( g_input_state.define_input( "turnLeft", "a", "a-up" ) );
                _inputs.push_back( g_input_state.define_input( "turnRight", "d", "d-up" ) );

                g_input_state.set( "turnLeft", slideLeftWASDSet );
                g_input_state.set( "turnRight", slideRightWASDSet );

                g_input_state.set( "slideLeft", false );
                g_input_state.set( "slideRight", false );
        }
        else
        {
                _inputs.push_back( g_input_state.define_input( "slideLeft", "a", "a-up" ) );
                _inputs.push_back( g_input_state.define_input( "slideRight", "d", "d-up" ) );

                g_input_state.set( "slideLeft", turnLeftWASDSet );
                g_input_state.set( "slideRight", turnRightWASDSet );

                g_input_state.set( "turnLeft", false );
                g_input_state.set( "turnRight", false );
        }
}