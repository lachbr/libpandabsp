#ifndef DISTRIBUTED_SMOOTH_NODE_H
#define DISTRIBUTED_SMOOTH_NODE_H

#include "distributedsmoothnode_base.h"
#include "distributednode.h"

#include <genericAsyncTask.h>
#include <smoothMover.h>
#include <configVariableDouble.h>
#include <configVariableBool.h>

extern ConfigVariableDouble max_future;
extern ConfigVariableDouble min_suggest_resync;
extern ConfigVariableBool   enable_smoothing;
extern ConfigVariableBool   enable_prediction;
extern ConfigVariableDouble lag;
extern ConfigVariableDouble prediction_lag;

NotifyCategoryDeclNoExport( distributedSmoothNode );

class EXPCL_PPDISTRIBUTED DistributedSmoothNode : public DistributedNode
{

        DClassDecl( DistributedSmoothNode, DistributedNode );

public:
        DistributedSmoothNode();
        virtual ~DistributedSmoothNode();

        virtual void generate();
        virtual void disable();
        virtual void delete_do();

        virtual void smooth_position();

        virtual bool wants_smoothing() const;

        void start_smooth();
        void stop_smooth();

        void set_smooth_wrt_reparents( bool flag );
        bool get_smooth_wrt_reparents() const;

        void force_to_true_position();
        void reload_position();

        void set_sm_stop( int timestamp );
        DCFuncDecl( set_sm_stop_field );

        void set_sm_pos( PN_stdfloat x, PN_stdfloat y, PN_stdfloat z, int timestamp );
        DCFuncDecl( set_sm_pos_field );
        DCFuncDecl( get_sm_pos_field );

        void set_sm_hpr( PN_stdfloat h, PN_stdfloat p, PN_stdfloat r, int timestamp );
        DCFuncDecl( set_sm_hpr_field );
        DCFuncDecl( get_sm_hpr_field );

        DCFuncDecl( set_curr_l_field );
        DCFuncDecl( get_curr_l_field );

        void b_clear_smoothing();
        void clear_smoothing();
        DCFuncDecl( clear_smoothing_field );

        void wrt_reparent_to( NodePath &parent );

        virtual void d_set_parent( int parent_token );

        void d_suggest_resync( DOID_TYPE av_id, int timestamp_a, int timestamp_b,
                               double server_time, PN_stdfloat uncertainty );
        void suggest_resync( DOID_TYPE av_id, int timestamp_a, int timestamp_b,
                             int server_time, uint16_t server_time_usec, PN_stdfloat uncertainty );
        DCFuncDecl( suggest_resync_field );

        void d_return_resync( DOID_TYPE av_id, int timestamp_b, double server_time, PN_stdfloat uncertainty );
        void return_resync( DOID_TYPE av_id, int timestamp_b, int server_time, uint16_t server_time_usec,
                            PN_stdfloat uncertainty );
        DCFuncDecl( return_resync_field );

        bool p2p_resync( DOID_TYPE av_id, int timestamp, double server_time,
                         PN_stdfloat uncertainty );

        void activate_smoothing( bool smoothing, bool prediction );

        DistributedSmoothNodeBase *get_snbase() const;

protected:
        SmoothMover _smoother;

private:
        void set_component_t_live( int timestamp );
        void set_component_t( int timestamp );

        static AsyncTask::DoneStatus do_smooth_task( GenericAsyncTask *task, void *data );

        void check_resume( int timestamp );

        bool _smooth_started;
        bool _local_control;
        bool _stopped;
        bool _smooth_wrt_reparents;

        PN_stdfloat _last_suggest_resync;

        PT( GenericAsyncTask ) _smooth_task;

        DistributedSmoothNodeBase *_snbase;

        InitTypeStart();

        DCFieldDefStart();

        DCFieldDef( set_sm_stop, set_sm_stop_field, nullptr );
        DCFieldDef( set_sm_pos, set_sm_pos_field, get_sm_pos_field );
        DCFieldDef( set_sm_hpr, set_sm_hpr_field, get_sm_hpr_field );
        DCFieldDef( set_curr_l, set_curr_l_field, get_curr_l_field );
        DCFieldDef( clear_smoothing, clear_smoothing_field, nullptr );
        DCFieldDef( suggest_resync, suggest_resync_field, nullptr );
        DCFieldDef( return_resync, return_resync_field, nullptr );

        DCFieldDefEnd();

        InitTypeEnd();
};

#endif // DISTRIBUTED_SMOOTH_NODE_H