#include "datagram_extensions.h"

void dg_add_server_control_header( Datagram &dg, uint16_t code )
{
        dg.add_int8( 1 );
        dg.add_uint64( CONTROL_CHANNEL );
        dg.add_uint16( code );
}

void dg_add_server_header( Datagram &dg, CHANNEL_TYPE chan, CHANNEL_TYPE sender, uint16_t code )
{
        dg.add_int8( 1 );
        dg.add_uint64( chan );
        dg.add_uint64( sender );
        dg.add_uint16( code );
}

void dg_add_old_server_header( Datagram &dg, CHANNEL_TYPE chan, CHANNEL_TYPE sender, uint16_t code )
{
        dg.add_uint64( chan );
        dg.add_uint64( sender );
        dg.add_uint64( 'A' );
        dg.add_uint16( code );
}