#ifndef __CONTROL_SYSTEM_BASE_H__
#define __CONTROL_SYSTEM_BASE_H__

#include "config_pandaplus.h"

#include <nodePath.h>

class EXPCL_PANDAPLUS ControlSystemBase
{
public:

        struct SpeedData
        {
                PN_stdfloat fwd_bck;
                PN_stdfloat rotate;
                PN_stdfloat slide;
        };

        virtual void disable_avatar_controls();
        virtual void enable_avatar_controls();
        virtual void set_collisions_active( bool flag = true );
        virtual void cleanup();
        virtual void set_avatar( NodePath &avatar );
        virtual void set_walk_speed( PN_stdfloat foward, PN_stdfloat jump,
                                     PN_stdfloat reverse, PN_stdfloat rotate );

        virtual SpeedData get_speeds() const;

        virtual bool get_is_airborne() const;

        virtual void set_tag( const string &key, const string &value );

        virtual void delete_collisions();

        virtual void place_on_floor();

protected:
        PN_stdfloat _avatar_control_forward_speed;
        PN_stdfloat _avatar_control_jump_force;
        PN_stdfloat _avatar_control_reverse_speed;
        PN_stdfloat _avatar_control_rotate_speed;

        PN_stdfloat _speed;
        PN_stdfloat _rotation_speed;
        PN_stdfloat _slide_speed;

        NodePath _avatar_np;

        bool _airborne;
        bool _collisions_active;
};

#endif // __CONTROL_SYSTEM_BASE_H__