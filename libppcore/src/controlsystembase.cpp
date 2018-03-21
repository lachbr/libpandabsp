#include "controlsystembase.h"
#include "pp_globals.h"

void ControlSystemBase::
enable_avatar_controls()
{
}

void ControlSystemBase::
disable_avatar_controls()
{
}

void ControlSystemBase::
set_collisions_active( bool flag )
{
        _collisions_active = flag;
}

void ControlSystemBase::
cleanup()
{
}

void ControlSystemBase::
set_avatar( NodePath &avatar )
{
        _avatar_np = avatar;
}

void ControlSystemBase::
set_walk_speed( PN_stdfloat forward, PN_stdfloat jump, PN_stdfloat reverse, PN_stdfloat rotate )
{
        _avatar_control_forward_speed = forward;
        _avatar_control_jump_force = jump;
        _avatar_control_reverse_speed = reverse;
        _avatar_control_rotate_speed = rotate;
}

ControlSystemBase::SpeedData ControlSystemBase::
get_speeds() const
{
        SpeedData data;
        data.fwd_bck = _speed;
        data.rotate = _rotation_speed;
        data.slide = _slide_speed;

        return data;
}

bool ControlSystemBase::
get_is_airborne() const
{
        return _airborne;
}

void ControlSystemBase::
set_tag( const string &key, const string &value ) {}

void ControlSystemBase::
delete_collisions()
{
}

void ControlSystemBase::
place_on_floor()
{
}