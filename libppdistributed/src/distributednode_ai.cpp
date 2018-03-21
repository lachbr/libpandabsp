#include "distributednode_ai.h"
#include "pp_globals.h"
#include "clientrepository.h"

DClassDef( DistributedNodeAI );

DistributedNodeAI::
DistributedNodeAI() :
        DistributedObjectAI(),
        NodePath( "distributed_node_AI" )
{
}

void DistributedNodeAI::
delete_do()
{
        if ( !is_empty() )
        {
                remove_node();
        }

        DistributedObjectAI::delete_do();
}

void DistributedNodeAI::
b_set_parent( int parent_token )
{
        set_parent( parent_token );
        d_set_parent( parent_token );
}

void DistributedNodeAI::
d_set_parent( int parent_token )
{
        BeginAIUpdate( "set_parent" );
        AddArg( int, parent_token );
        SendAIUpdate( "set_parent" );
}

void DistributedNodeAI::
set_parent( int parent_token )
{
        do_set_parent( parent_token );
}

void DistributedNodeAI::
do_set_parent( int parent_token )
{
        // not implemented.
}

void DistributedNodeAI::
set_parent_field( DCFuncArgs )
{
        ( ( DistributedNodeAI * )data )->set_parent( packer.unpack_int() );
}

//void DistributedNodeAI::GetParentField( DCFuncArgs )
//{
//        packer.pack_int(((DistributedNodeAI *) )
//}

void DistributedNodeAI::
d_set_x( float x )
{
        BeginAIUpdate( "set_x" );
        AddArg( double, x );
        SendAIUpdate( "set_x" );
}

void DistributedNodeAI::
set_x_field( DCFuncArgs )
{
        ( ( DistributedNodeAI * )data )->set_x( packer.unpack_double() );
}

void DistributedNodeAI::
get_x_field( DCFuncArgs )
{
        packer.pack_double( ( ( DistributedNodeAI * )data )->get_x() );
}

void DistributedNodeAI::
d_set_y( float y )
{
        BeginAIUpdate( "set_y" );
        AddArg( double, y );
        SendAIUpdate( "set_y" );
}

void DistributedNodeAI::
set_y_field( DCFuncArgs )
{
        ( ( DistributedNodeAI * )data )->set_y( packer.unpack_double() );
}

void DistributedNodeAI::
get_y_field( DCFuncArgs )
{
        packer.pack_double( ( ( DistributedNodeAI * )data )->get_y() );
}

void DistributedNodeAI::
d_set_z( float z )
{
        BeginAIUpdate( "set_z" );
        AddArg( double, z );
        SendAIUpdate( "set_z" );
}

void DistributedNodeAI::
set_z_field( DCFuncArgs )
{
        ( ( DistributedNodeAI * )data )->set_z( packer.unpack_double() );
}

void DistributedNodeAI::
get_z_field( DCFuncArgs )
{
        packer.pack_double( ( ( DistributedNodeAI * )data )->get_z() );
}

void DistributedNodeAI::
d_set_h( float h )
{
        BeginAIUpdate( "set_h" );
        AddArg( double, h );
        SendAIUpdate( "set_h" );
}

void DistributedNodeAI::
set_h_field( DCFuncArgs )
{
        ( ( DistributedNodeAI * )data )->set_h( packer.unpack_double() );
}

void DistributedNodeAI::
get_h_field( DCFuncArgs )
{
        packer.pack_double( ( ( DistributedNodeAI * )data )->get_h() );
}

void DistributedNodeAI::
d_set_p( float p )
{
        BeginAIUpdate( "set_p" );
        AddArg( double, p );
        SendAIUpdate( "set_p" );
}

void DistributedNodeAI::
set_p_field( DCFuncArgs )
{
        ( ( DistributedNodeAI * )data )->set_p( packer.unpack_double() );
}

void DistributedNodeAI::
get_p_field( DCFuncArgs )
{
        packer.pack_double( ( ( DistributedNodeAI * )data )->get_p() );
}

void DistributedNodeAI::
d_set_r( float r )
{
        BeginAIUpdate( "set_r" );
        AddArg( double, r );
        SendAIUpdate( "set_r" );
}

void DistributedNodeAI::
set_r_field( DCFuncArgs )
{
        ( ( DistributedNodeAI * )data )->set_r( packer.unpack_double() );
}

void DistributedNodeAI::
get_r_field( DCFuncArgs )
{
        packer.pack_double( ( ( DistributedNodeAI * )data )->get_r() );
}