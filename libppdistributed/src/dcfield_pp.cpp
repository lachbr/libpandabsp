/**
* @file dcField_pp.cxx
* @author Brian Lach
* @date 2017-02-07
*/

#include "dcfield_pp.h"
#include "config_pandaplus.h"

DCFieldPP::DCFieldPP( DCAtomicField *field ) :
        _field( field )
{
}

void DCFieldPP::
set_setter_func( DCFunc *func )
{
        _set_func = func;
}

DCFunc *DCFieldPP::
get_setter_func()
{
        return _set_func;
}

void DCFieldPP::
set_getter_func( DCFunc *func )
{
        _get_func = func;
}

DCFunc *DCFieldPP::
get_getter_func() const
{
        return _get_func;
}

DCPacker DCFieldPP::
start_client_format_update( uint32_t do_id )
{
        DCPacker packer;
        packer.raw_pack_uint16( CLIENT_OBJECT_SET_FIELD );
        packer.raw_pack_uint32( do_id );
        packer.raw_pack_uint16( _field->get_number() );
        packer.begin_pack( _field );
        packer.push();
        return packer;
}

Datagram DCFieldPP::
end_client_format_update( DCPacker &packer )
{
        packer.pop();
        bool result = packer.end_pack();
        if ( !result )
        {
                cout << "DCFieldPP ERROR: end_pack() failed for " << _field->get_name() << endl;
                return Datagram();
        }
        return Datagram( packer.get_data(), packer.get_length() );
}

DCPacker DCFieldPP::
start_ai_format_update( uint32_t do_id, uint32_t to_id, uint32_t from_id )
{
        DCPacker packer;
        packer.raw_pack_uint8( 1 );
        packer.RAW_PACK_CHANNEL( to_id );
        packer.RAW_PACK_CHANNEL( from_id );
        packer.raw_pack_uint16( STATESERVER_OBJECT_SET_FIELD );
        packer.raw_pack_uint32( do_id );
        packer.raw_pack_uint16( _field->get_number() );
        packer.begin_pack( _field );
        packer.push();
        return packer;
}

Datagram DCFieldPP::
end_ai_format_update( DCPacker &packer )
{
        // It does the same thing.
        return end_client_format_update( packer );
}

DCPacker DCFieldPP::
start_ai_format_update_msg_type( uint32_t do_id, uint32_t to_id, uint32_t from_id, int msgtype )
{
        DCPacker packer;
        packer.raw_pack_uint8( 1 );
        packer.RAW_PACK_CHANNEL( to_id );
        packer.RAW_PACK_CHANNEL( from_id );
        packer.raw_pack_uint16( msgtype );
        packer.raw_pack_uint32( do_id );
        packer.raw_pack_uint16( _field->get_number() );
        packer.begin_pack( _field );
        packer.push();
        return packer;
}

Datagram DCFieldPP::
end_ai_format_update_msg_type( DCPacker &packer )
{
        return end_client_format_update( packer );
}

DCAtomicField *DCFieldPP::
get_field() const
{
        return _field;
}