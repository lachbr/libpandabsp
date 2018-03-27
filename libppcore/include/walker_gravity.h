#ifndef WALKER_GRAVITY_H
#define WALKER_GRAVITY_H

#include "config_pandaplus.h"
#include "walker.h"

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

class EXPCL_PANDAPLUS GravityWalker : public BaseWalker
{
public:

        GravityWalker( PN_stdfloat grav = 64.348, PN_stdfloat standable_ground = 0.707,
                       PN_stdfloat hard_landing_force = 16.0, bool legacy_lifter = false );

        virtual void setup_ray( const BitMask32 &bitmask, PN_stdfloat offset, PN_stdfloat reach );

        virtual void set_gravity( PN_stdfloat gravity );

        virtual void initialize_collisions( CollisionTraverser *trav, const NodePath &avatar_np,
                                    PN_stdfloat avatar_radius, PN_stdfloat floor_offset, PN_stdfloat reach );

        PN_stdfloat get_airborne_height() const;

        virtual void delete_collisions();

        virtual void set_collisions_active( bool active = true );

        virtual void one_time_collide();

        void start_jump_delay( PN_stdfloat delay );

        void add_blast_force( LVecBase3f &vector );

        void do_delta_pos();

        void set_prior_parent_vector();

        void reset();

        virtual void disable_avatar_controls();

        virtual void flush_event_handlers();

private:

        void set_may_jump();
        static AsyncTask::DoneStatus jump_task( GenericAsyncTask *task, void *data );

        void handle_avatar_controls();
        static AsyncTask::DoneStatus av_ctrl_task( GenericAsyncTask *task, void *data );

        PN_stdfloat _old_dt;

        PT( GenericAsyncTask ) _jump_delay_task;
        PT( GenericAsyncTask ) _indicator_task;

        bool _legacy_lifter;
        bool _falling;
        bool _need_to_delta_pos;

        LVector3f _prior_parent;
        LVector3f _old_pos_delta;

        PT( CollisionHandlerGravity ) _lifter;
};

#endif // WALKER_GRAVITY_H