#ifndef __CONTROL_MANAGER_H__
#define __CONTROL_MANAGER_H__

#define COLL_HANDLER_RAY_START 4000.0

#include <configVariableBool.h>
#include <nodePath.h>

#include "controlsystembase.h"
#include "inputstate.h"

class EXPCL_PANDAPLUS ControlManager
{
public:
        static ConfigVariableBool _want_wasd;
        ControlManager( bool enable_now = true, bool pass_messages_through = false );

        void add( ControlSystemBase &controls, const string &name = "basic" );

        ControlSystemBase &get( const string &name );
        void remove( const string &name );
        void use( const string &name, NodePath &avatar );

        void set_speeds( PN_stdfloat fwd, PN_stdfloat jump, PN_stdfloat rev, PN_stdfloat rot );

        void cleanup();

        ControlSystemBase::SpeedData get_speeds() const;

        bool get_is_airborne() const;

        void set_tag( const string &key, const string &value );

        void delete_collisions();

        void collisions_on();
        void collisions_off();

        void place_on_floor();

        void enable();
        void disable();
        void stop();

        void disable_avatar_jump();
        void enable_avatar_jump();

        void set_wasd_turn( bool turn );

private:

        bool control_sys_exists( const string &name );

        bool _pass_messages_through;
        pvector<string> _inputs;

        ControlSystemBase *_current_controls;
        string _current_controls_name;

        bool _wasd_turn;

        pmap<string, ControlSystemBase> _controls;

        bool _enabled;
};

#endif // __CONTROL_MANAGER_H__