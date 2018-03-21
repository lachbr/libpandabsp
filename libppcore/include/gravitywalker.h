#ifndef __GRAVITY_WALKER_H__
#define __GRAVITY_WALKER_H__

#include "config_pandaplus.h"
#include "controlsystembase.h"

#include <genericAsyncTask.h>
#include <lvecBase3.h>
#include <nodePath.h>
#include <bitMask.h>
#include <collisionRay.h>
#include <collisionHandlerFluidPusher.h>
#include <collisionHandlerGravity.h>
#include <collisionHandlerPusher.h>
#include <collisionHandlerEvent.h>
#include <collisionTraverser.h>
#include <configVariableBool.h>

class EXPCL_PANDAPLUS GravityWalker : public ControlSystemBase
{
public:

        GravityWalker( PN_stdfloat grav = 64.348, PN_stdfloat standable_ground = 0.707,
                       PN_stdfloat hard_landing_force = 16.0, bool legacy_lifter = false );

        void setup_ray( BitMask32 &bitmask, PN_stdfloat floor_offset, PN_stdfloat reach );
        void setup_wall_sphere( BitMask32 &bitmask, PN_stdfloat avatar_radius );
        void setup_event_sphere( BitMask32 &bitmask, PN_stdfloat avatar_radius );
        void setup_floor_sphere( BitMask32 &bitmask, PN_stdfloat avatar_radius );

        void set_wall_bitmask( BitMask32 &bitmask );
        void set_floor_bitmask( BitMask32 &bitmask );
        void set_event_bitmask( BitMask32 &bitmask );

        void swap_floor_bitmask( BitMask32 &old_mask, BitMask32 &new_mask );

        void set_gravity( PN_stdfloat gravity );
        PN_stdfloat get_gravity() const;

        void initialize_collisions( CollisionTraverser *trav, NodePath &avatar_nodepath,
                                    PN_stdfloat avatar_radius, PN_stdfloat floor_offset, PN_stdfloat reach );

        void set_tag( const string &key, const string &value );

        PN_stdfloat get_airborne_height() const;

        void set_avatar_physics_indicator( NodePath &indicator );

        void delete_collisions();

        void set_collisions_active( bool active = true );
        bool get_collisions_active() const;

        void place_on_floor();
        void one_time_collide();

        void start_jump_delay( PN_stdfloat delay );

        void add_blast_force( LVecBase3f &vector );

        string get_hard_land_event() const;
        string get_land_event() const;
        string get_jump_start_event() const;
        string get_moving_event() const;

        void do_delta_pos();

        void set_prior_parent_vector();

        void reset();

        const LVector3f &get_velocity() const;

        void enable_avatar_controls();
        void disable_avatar_controls();

        void flush_event_handlers();

        void set_collision_ray_height( PN_stdfloat height );

private:

        void set_may_jump();
        static AsyncTask::DoneStatus jump_task( GenericAsyncTask *task, void *data );

        void handle_avatar_controls();
        static AsyncTask::DoneStatus av_ctrl_task( GenericAsyncTask *task, void *data );

        PN_stdfloat _gravity;
        PN_stdfloat _standable_ground;
        PN_stdfloat _hard_landing_force;

        PN_stdfloat _avatar_radius;

        PN_stdfloat _old_dt;

        PT( GenericAsyncTask ) _jump_delay_task;
        PT( GenericAsyncTask ) _controls_task;
        PT( GenericAsyncTask ) _indicator_task;

        bool _legacy_lifter;
        bool _may_jump;
        bool _falling;
        bool _need_to_delta_pos;
        bool _moving;

        LVector3f _prior_parent;
        LVector3f _old_pos_delta;
        LVector3f _vel;

        NodePath _avatar_np;

        PT( CollisionRay ) _coll_ray;
        NodePath _coll_ray_np;

        PT( CollisionHandlerGravity ) _lifter;
        PT( CollisionHandlerPusher ) _pusher;
        PT( CollisionHandlerFluidPusher ) _pusher_f;
        PT( CollisionHandlerEvent ) _event;
        PT( CollisionHandlerPusher ) _pusher_floor;

        NodePath _cwallsphere_np;
        NodePath _ceventsphere_np;
        NodePath _cfloorsphere_np;

        BitMask32 _wall_bitmask;
        BitMask32 _floor_bitmask;
        BitMask32 _event_bitmask;

        CollisionTraverser *_ctrav;
};

#endif // __GRAVITY_WALKER_H__