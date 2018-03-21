#ifndef DISTRIBUTED_SMOOTH_NODE_AI_H
#define DISTRIBUTED_SMOOTH_NODE_AI_H

#include "distributedsmoothnode_base.h"
#include "distributednode_ai.h"

class EXPCL_PPDISTRIBUTED DistributedSmoothNodeAI : public DistributedNodeAI
{
        DClassDecl( DistributedSmoothNodeAI, DistributedNodeAI );

public:
        DistributedSmoothNodeAI();
        virtual ~DistributedSmoothNodeAI();

        virtual void generate();
        virtual void disable();
        virtual void delete_do();

        void set_sm_stop( int timestamp );
        DCFuncDecl( set_sm_stop_field );

        void set_sm_pos( float x, float y, float z, int timestamp );
        DCFuncDecl( set_sm_pos_field );
        DCFuncDecl( get_sm_pos_field );

        void set_sm_hpr( float h, float p, float r, int timestamp );
        DCFuncDecl( set_sm_hpr_field );
        DCFuncDecl( get_sm_hpr_field );

        DCFuncDecl( set_curr_l_field );
        DCFuncDecl( get_curr_l_field );

        void b_clear_smoothing();
        void clear_smoothing();
        DCFuncDecl( clear_smoothing_field );

        DistributedSmoothNodeBase *get_snbase() const;

protected:
        DistributedSmoothNodeBase *_snbase;

        InitTypeStart();

        DCFieldDefStart();

        DCFieldDef( set_sm_stop, set_sm_stop_field, NULL );
        DCFieldDef( set_sm_pos, set_sm_pos_field, get_sm_pos_field );
        DCFieldDef( set_sm_hpr, set_sm_hpr_field, get_sm_hpr_field );
        DCFieldDef( set_curr_l, set_curr_l_field, get_curr_l_field );
        DCFieldDef( clear_smoothing, clear_smoothing_field, NULL );

        DCFieldDefEnd();

        InitTypeEnd();

};

#endif // DISTRIBUTED_SMOOTH_NODE_AI_H