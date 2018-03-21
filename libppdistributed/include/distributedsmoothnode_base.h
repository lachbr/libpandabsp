#ifndef DISTRIBUTED_SMOOTH_NODE_BASE_H
#define DISTRIBUTED_SMOOTH_NODE_BASE_H

#include "pp_dcbase.h"

#include <nodePath.h>
#include <genericAsyncTask.h>

class DCClassPP;
class ClientRepository;

class EXPCL_PPDISTRIBUTED DistributedSmoothNodeBase
{
public:
        DistributedSmoothNodeBase();
        ~DistributedSmoothNodeBase();

        void set_repository( ClientRepository *repo, bool is_ai, CHANNEL_TYPE ai_id );

        void initialize( const NodePath &np, DCClassPP *dclass, DOID_TYPE do_id );

        void send_everything();
        void broadcast_pos_hpr_full();

        void d_set_sm_stop();

        void d_set_sm_hpr( float h, float p, float r );

        void d_set_sm_pos( float x, float y, float z );

        void d_set_curr_l( uint64_t l );
        void set_curr_l( uint64_t l );

        void generate();
        void disable();

        void d_clear_smoothing();

        string get_pos_hpr_broadcast_task_name() const;

        void set_pos_hpr_broadcast_period( float period );
        float get_pos_hpr_broadcast_period() const;

        void stop_pos_hpr_broadcast();

        bool pos_hpr_broadcast_started() const;

        bool want_smooth_broadcast_task() const;

        virtual void start_pos_hpr_broadcast( float period = 0.2, bool stagger = false );

        virtual void send_current_position();

private:
        static AsyncTask::DoneStatus pos_hpr_broadcast_task( GenericAsyncTask *task, void *data );

        static bool only_changed( int flags, int compare );

        enum Flags
        {
                F_new_pos = 0x01,
                F_new_hpr = 0x02
        };

        NodePath _np;
        DCClassPP *_dclass;
        DOID_TYPE _do_id;
        ClientRepository *_repo;
        bool _is_ai;
        DOID_TYPE _ai_id;

        LPoint3 _store_xyz;
        LVecBase3 _store_hpr;

        bool _store_stop;

        uint64_t _curr_l[2];

        PT( GenericAsyncTask ) _broadcast_task;

        float _broadcast_period;
};

#endif // DISTRIBUTED_SMOOTH_NODE_BASE_H