#ifndef DISTRIBUTED_NODE_AI_H
#define DISTRIBUTED_NODE_AI_H

#include "distributedobject_ai.h"
#include <nodePath.h>

class EXPCL_PPDISTRIBUTED DistributedNodeAI : public DistributedObjectAI, public NodePath
{
        DClassDecl( DistributedNodeAI, DistributedObjectAI );

public:
        DistributedNodeAI();

        virtual void delete_do();

        void b_set_parent( int parent_token );
        void d_set_parent( int parent_token );
        void set_parent( int parent_token );
        DCFuncDecl( set_parent_field );
        virtual void do_set_parent( int parent_token );

        void d_set_x( PN_stdfloat x );
        void d_set_y( PN_stdfloat y );
        void d_set_z( PN_stdfloat z );
        void d_set_h( PN_stdfloat h );
        void d_set_p( PN_stdfloat p );
        void d_set_r( PN_stdfloat r );

        DCFuncDecl( set_x_field );
        DCFuncDecl( set_y_field );
        DCFuncDecl( set_z_field );
        DCFuncDecl( set_h_field );
        DCFuncDecl( set_p_field );
        DCFuncDecl( set_r_field );

        DCFuncDecl( get_x_field );
        DCFuncDecl( get_y_field );
        DCFuncDecl( get_z_field );
        DCFuncDecl( get_h_field );
        DCFuncDecl( get_p_field );
        DCFuncDecl( get_r_field );

        InitTypeStart();

        DCFieldDefStart();

        DCFieldDef( set_x, set_x_field, get_x_field );
        DCFieldDef( set_y, set_y_field, get_y_field );
        DCFieldDef( set_z, set_z_field, get_z_field );
        DCFieldDef( set_h, set_h_field, get_h_field );
        DCFieldDef( set_p, set_p_field, get_p_field );
        DCFieldDef( set_r, set_r_field, get_r_field );
        DCFieldDef( set_parent, set_parent_field, nullptr );

        DCFieldDefEnd();

        InitTypeEnd();
};

#endif // DISTRIBUTED_NODE_AI_H