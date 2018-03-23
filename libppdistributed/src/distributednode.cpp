#include "distributednode.h"

DClassDef( DistributedNode );

#include "pp_globals.h"
#include "clientrepository.h"

DistributedNode::DistributedNode() :
        DistributedObject(),
        NodePath( "distributed_node" )
{
        _got_string_parent_token = false;
}


void DistributedNode::disable()
{
        if ( _active_state == DistributedObject::ES_disabled )
        {
                if ( !is_empty() )
                {
                        reparent_to( g_hidden );
                }
                DistributedObject::disable();
        }
}

void DistributedNode::delete_do()
{
        if ( !is_empty() )
        {
                remove_node();
        }
        DistributedObject::delete_do();
}

void DistributedNode::generate()
{
        DistributedObject::generate();
        _got_string_parent_token = false;
}

void DistributedNode::b_set_parent( int parent_token )
{
        set_parent( parent_token );
        d_set_parent( parent_token );
}

void DistributedNode::d_set_parent( int parent_token )
{
        BeginCLUpdate( "set_parent" );
        AddArg( int, parent_token );
        SendCLUpdate( "set_parent" );
}

void DistributedNode::set_parent( int parent_token )
{
        bool just_got_required_parent_as_str = ( ( !is_generated() ) && _got_string_parent_token );
        if ( !just_got_required_parent_as_str )
        {
                if ( parent_token != 0 )
                {
                        do_set_parent( parent_token );
                }
        }
        _got_string_parent_token = false;
}

void DistributedNode::do_set_parent( int parent_token )
{
        if ( !is_disabled() )
        {
                g_cr->get_parent_mgr().request_reparent( this, parent_token );
        }
}

void DistributedNode::set_parent_field( DCFuncArgs )
{
        ( (DistributedNode *)data )->set_parent( packer.unpack_int() );
}

void DistributedNode::d_set_x( PN_stdfloat x )
{
        BeginCLUpdate( "set_x" );
        AddArg( double, x );
        SendCLUpdate( "set_x" );
}

void DistributedNode::set_x_field( DCFuncArgs )
{
        ( (DistributedNode *)data )->set_x( packer.unpack_double() );
}

void DistributedNode::get_x_field( DCFuncArgs )
{
        packer.pack_double( ( (DistributedNode *)data )->get_x() );
}

void DistributedNode::d_set_y( PN_stdfloat y )
{
        BeginCLUpdate( "set_y" );
        AddArg( double, y );
        SendCLUpdate( "set_y" );
}

void DistributedNode::set_y_field( DCFuncArgs )
{
        ( (DistributedNode *)data )->set_y( packer.unpack_double() );
}

void DistributedNode::get_y_field( DCFuncArgs )
{
        packer.pack_double( ( (DistributedNode *)data )->get_y() );
}

void DistributedNode::d_set_z( PN_stdfloat z )
{
        BeginCLUpdate( "set_z" );
        AddArg( double, z );
        SendCLUpdate( "set_z" );
}

void DistributedNode::set_z_field( DCFuncArgs )
{
        ( (DistributedNode *)data )->set_z( packer.unpack_double() );
}

void DistributedNode::get_z_field( DCFuncArgs )
{
        packer.pack_double( ( (DistributedNode *)data )->get_z() );
}

void DistributedNode::d_set_h( PN_stdfloat h )
{
        BeginCLUpdate( "set_h" );
        AddArg( double, h );
        SendCLUpdate( "set_h" );
}

void DistributedNode::set_h_field( DCFuncArgs )
{
        ( (DistributedNode *)data )->set_h( packer.unpack_double() );
}

void DistributedNode::get_h_field( DCFuncArgs )
{
        packer.pack_double( ( (DistributedNode *)data )->get_h() );
}

void DistributedNode::d_set_p( PN_stdfloat p )
{
        BeginCLUpdate( "set_p" );
        AddArg( double, p );
        SendCLUpdate( "set_p" );
}

void DistributedNode::set_p_field( DCFuncArgs )
{
        ( (DistributedNode *)data )->set_p( packer.unpack_double() );
}

void DistributedNode::get_p_field( DCFuncArgs )
{
        packer.pack_double( ( (DistributedNode *)data )->get_p() );
}

void DistributedNode::d_set_r( PN_stdfloat r )
{
        BeginCLUpdate( "set_r" );
        AddArg( double, r );
        SendCLUpdate( "set_r" );
}

void DistributedNode::set_r_field( DCFuncArgs )
{
        ( (DistributedNode *)data )->set_r( packer.unpack_double() );
}

void DistributedNode::get_r_field( DCFuncArgs )
{
        packer.pack_double( ( (DistributedNode *)data )->get_r() );
}