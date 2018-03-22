#include "distributedsmoothnode_ai.h"
#include "pp_globals.h"
#include "clientrepository.h"

DClassDef( DistributedSmoothNodeAI );

DistributedSmoothNodeAI::DistributedSmoothNodeAI() :
        DistributedNodeAI()
{
        _snbase = new DistributedSmoothNodeBase;
}

DistributedSmoothNodeAI::~DistributedSmoothNodeAI()
{
        delete _snbase;
}

DistributedSmoothNodeBase *DistributedSmoothNodeAI::get_snbase() const
{
        return _snbase;
}

void DistributedSmoothNodeAI::generate()
{
        DistributedNodeAI::generate();
        _snbase->generate();
        _snbase->set_repository( g_air, true, g_air->get_our_channel() );
}

void DistributedSmoothNodeAI::disable()
{
        _snbase->disable();
        DistributedNodeAI::disable();
}

void DistributedSmoothNodeAI::delete_do()
{
        DistributedNodeAI::delete_do();
}

void DistributedSmoothNodeAI::set_sm_stop( int timestamp )
{
}

void DistributedSmoothNodeAI::set_sm_stop_field( DCFuncArgs )
{
        ( (DistributedSmoothNodeAI *)data )->set_sm_stop( packer.unpack_int() );
}

void DistributedSmoothNodeAI::set_sm_pos( float x, float y, float z, int timestamp )
{
        set_pos( x, y, z );
}

void DistributedSmoothNodeAI::set_sm_pos_field( DCFuncArgs )
{
        DistributedSmoothNodeAI *obj = (DistributedSmoothNodeAI *)data;
        float x = packer.unpack_double();
        float y = packer.unpack_double();
        float z = packer.unpack_double();
        int ts = packer.unpack_int();
        obj->set_sm_pos( x, y, z, ts );
}

void DistributedSmoothNodeAI::get_sm_pos_field( DCFuncArgs )
{
        DistributedSmoothNodeAI *obj = (DistributedSmoothNodeAI *)data;
        packer.pack_double( obj->get_x() );
        packer.pack_double( obj->get_y() );
        packer.pack_double( obj->get_z() );
        packer.pack_int( 0 );
}

void DistributedSmoothNodeAI::set_sm_hpr( float h, float p, float r, int timestamp )
{
        set_hpr( h, p, r );
}

void DistributedSmoothNodeAI::set_sm_hpr_field( DCFuncArgs )
{
        float h = packer.unpack_double();
        float p = packer.unpack_double();
        float r = packer.unpack_double();
        int ts = packer.unpack_int();
        ( (DistributedSmoothNodeAI *)data )->set_sm_hpr( h, p, r, ts );
}

void DistributedSmoothNodeAI::get_sm_hpr_field( DCFuncArgs )
{
        DistributedSmoothNodeAI *obj = (DistributedSmoothNodeAI *)data;
        packer.pack_double( obj->get_h() );
        packer.pack_double( obj->get_p() );
        packer.pack_double( obj->get_r() );
        packer.pack_int( 0 );
}

void DistributedSmoothNodeAI::b_clear_smoothing()
{
        _snbase->d_clear_smoothing();
        clear_smoothing();
}

void DistributedSmoothNodeAI::clear_smoothing()
{
}

void DistributedSmoothNodeAI::clear_smoothing_field( DCFuncArgs )
{
        ( (DistributedSmoothNodeAI *)data )->clear_smoothing();
}

void DistributedSmoothNodeAI::set_curr_l_field( DCFuncArgs )
{
        DistributedSmoothNodeAI *obj = (DistributedSmoothNodeAI *)data;

        CHANNEL_TYPE l = packer.unpack_uint64();
        int ts = packer.unpack_int();
        obj->_snbase->set_curr_l( l );
}

void DistributedSmoothNodeAI::get_curr_l_field( DCFuncArgs )
{
        DistributedSmoothNodeAI *obj = (DistributedSmoothNodeAI *)data;
        if ( obj->_zone_id )
        {
                packer.pack_uint64( obj->_zone_id );
        }
        else
        {
                packer.pack_uint64( 0 );
        }
        packer.pack_int( 0 );
}