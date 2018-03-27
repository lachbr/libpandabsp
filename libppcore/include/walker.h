#ifndef WALKER_H
#define WALKER_H

#include "config_pandaplus.h"

#include <nodePath.h>
#include <collisionRay.h>
#include <collisionNode.h>
#include <collisionTraverser.h>
#include <genericAsyncTask.h>
#include <collisionHandlerPusher.h>

class EXPCL_PANDAPLUS BaseWalker
{
public:
        struct InputData
        {
                bool fwd;
                bool rev;
                bool sl;
                bool sr;
                bool tl;
                bool tr;
                bool jump;
        };

        BaseWalker();
        BaseWalker( PN_stdfloat gravity, PN_stdfloat standable_ground,
                    PN_stdfloat hard_landing_force );

        // Sets the friction ratio from 0-1. 1 means full friction (no sliding).
        // The closer the number gets to 0, the more smoothing/sliding occurs.
        void set_friction( PN_stdfloat friction );
        PN_stdfloat get_friction() const;

        virtual void reset();

        virtual void disable_avatar_controls();
        virtual void enable_avatar_controls();
        virtual void cleanup();
        virtual void set_avatar( const NodePath &avatar );
        virtual void set_walk_speed( PN_stdfloat foward, PN_stdfloat jump,
                                     PN_stdfloat reverse, PN_stdfloat rotate );

        virtual LVector3 get_speeds() const;

        virtual bool get_is_airborne() const;

        void set_tag( const string &key, const string &value );

        virtual void place_on_floor();

        virtual void setup_ray( const BitMask32 &bitmask );
        virtual void setup_wall_sphere( const BitMask32 &bitmask, PN_stdfloat avatar_radius );
        virtual void setup_event_sphere( const BitMask32 &bitmask, PN_stdfloat avatar_radius );
        virtual void setup_floor_sphere( const BitMask32 &bitmask, PN_stdfloat avatar_radius );
        virtual PN_stdfloat get_airborne_height() const;

        virtual void handle_avatar_controls();

        virtual void flush_event_handlers();

        void swap_floor_bitmask( BitMask32 &old_mask, BitMask32 &new_mask );

        void set_wall_bitmask( const BitMask32 &bitmask );
        void set_floor_bitmask( const BitMask32 &bitmask );
        void set_event_bitmask( const BitMask32 &bitmask );

        virtual void set_gravity( PN_stdfloat gravity );
        PN_stdfloat get_gravity() const;

        virtual void delete_collisions();

        virtual void set_collisions_active( bool active = true );
        bool get_collisions_active() const;

        virtual void one_time_collide();

        void set_collision_ray_height( PN_stdfloat height );

        virtual void initialize_collisions( CollisionTraverser *trav, const NodePath &avatar_np,
                                            PN_stdfloat avatar_radius = 1.4, PN_stdfloat flr_offset = 1.0,
                                            PN_stdfloat reach = 1.0 );

        virtual LVector3 get_velocity() const;

        string get_hard_land_event() const;
        string get_land_event() const;
        string get_jump_start_event() const;
        string get_moving_event() const;

private:
        static AsyncTask::DoneStatus tick_task( GenericAsyncTask *task, void *data );

protected:
        void update_speeds(const InputData &data, bool apply_friction = true);
        InputData get_input_data() const;

protected:
        PN_stdfloat _avatar_control_forward_speed;
        PN_stdfloat _avatar_control_jump_force;
        PN_stdfloat _avatar_control_reverse_speed;
        PN_stdfloat _avatar_control_rotate_speed;

        PN_stdfloat _friction;
        LVector3 _last_speeds;

        PN_stdfloat _gravity;
        PN_stdfloat _standable_ground;
        PN_stdfloat _hard_landing_force;

        BitMask32 _wall_bitmask;
        BitMask32 _floor_bitmask;
        BitMask32 _event_bitmask;

        LVector3 _speeds;
        LVector3 _vel;

        NodePath _avatar_np;

        bool _airborne;
        bool _may_jump;
        bool _moving;
        bool _collisions_active;

        PT( CollisionRay ) _coll_ray;
        NodePath _coll_ray_np;
        NodePath _cwallsphere_np;
        NodePath _ceventsphere_np;

        PN_stdfloat _avatar_radius;

        CollisionTraverser *_ctrav;

        PT( CollisionHandlerPusher ) _pusher;
        PT( CollisionHandlerEvent ) _event;
        PT( CollisionHandlerPusher ) _pusher_floor;

        NodePath _cfloorsphere_np;

        PT( GenericAsyncTask ) _tick_task;
};

#endif // WALKER_H