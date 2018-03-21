#include "dcclass_pp.h"
#include "config_pandaplus.h"
#include "pp_dcbase.h"

#include <dcPacker.h>
#include <string>
#include <vector>

DCClassPP::
DCClassPP( DCClass *dclass ) :
        _dclass( dclass )
{
}

void DCClassPP::
set_obj_singleton( DistributedObjectBase *singleton )
{
        _obj_singleton = singleton;
}

DistributedObjectBase *DCClassPP::
get_obj_singleton()
{
        return _obj_singleton;
}

//DCFieldPP *DCClassPP::
//get_field_by_index(int n) {
//
//}

void DCClassPP::
receive_update( DistributedObjectBase *obj, DatagramIterator &di ) const
{
        DCPacker packer;
        const char *data = ( const char * )di.get_datagram().get_data();
        packer.set_unpack_data( data + di.get_current_index(),
                                di.get_remaining_size(), false );

        int field_id = packer.raw_unpack_uint16();
        DCFieldPP *field = get_field_wrapper( _dclass->get_field_by_index( field_id ) );
        if ( field == ( DCFieldPP * )NULL )
        {
                return;
        }

        packer.begin_unpack( field->get_field() );
        packer.push();
        // call the setter function
        ( *field->get_setter_func() )( packer, obj );
        packer.pop();
        packer.end_unpack();

        di.skip_bytes( packer.get_num_unpacked_bytes() );
}

void DCClassPP::
receive_update_broadcast_required( DistributedObjectBase *obj, DatagramIterator &di ) const
{
        DCPacker packer;
        const char *data = ( const char * )di.get_datagram().get_data();
        packer.set_unpack_data( data + di.get_current_index(),
                                di.get_remaining_size(), false );

        int num_fields = _dclass->get_num_inherited_fields();
        for ( int i = 0; i < num_fields; i++ )
        {
                DCField *dcfield = _dclass->get_inherited_field( i );
                if ( dcfield == ( DCField * )NULL )
                {
                        continue;
                }

                DCFieldPP *field = get_field_wrapper( dcfield );
                if ( field == ( DCFieldPP * )NULL )
                {
                        continue;
                }

                if ( field->get_field()->as_molecular_field() == ( DCMolecularField * )NULL &&
                     field->get_field()->is_required() && field->get_field()->is_broadcast() )
                {
                        packer.begin_unpack( field->get_field() );
                        packer.push();
                        ( *field->get_setter_func() )( packer, obj );
                        packer.pop();
                        if ( !packer.end_unpack() )
                        {
                                cout << "DCClassPP ERROR (receive_update_broadcast_required): Unpack failed for "
                                     << dcfield->get_name() << endl;
                        }
                }
        }

        di.skip_bytes( packer.get_num_unpacked_bytes() );
}

void DCClassPP::
receive_update_broadcast_required_owner( DistributedObjectBase *obj, DatagramIterator &di ) const
{
        DCPacker packer;
        const char *data = ( const char * )di.get_datagram().get_data();
        packer.set_unpack_data( data + di.get_current_index(),
                                di.get_remaining_size(), false );

        int num_fields = _dclass->get_num_inherited_fields();
        for ( int i = 0; i < num_fields; ++i )
        {
                DCField *dcfield = _dclass->get_inherited_field( i );
                DCFieldPP *field = get_field_wrapper( dcfield );
                if ( field == ( DCFieldPP * )NULL )
                {
                        continue;
                }
                if ( field->get_field()->as_molecular_field() == ( DCMolecularField * )NULL &&
                     field->get_field()->is_required() )
                {
                        packer.begin_unpack( field->get_field() );
                        packer.push();
                        if ( field->get_field()->is_ownrecv() )
                        {
                                ( *field->get_setter_func() )( packer, obj );
                        }
                        else
                        {
                                // It's not an ownrecv field; skip over it.  It's difficult to filter
                                // this on the server, ask Roger for the reason.
                                packer.unpack_skip();
                        }
                        packer.pop();
                        if ( !packer.end_unpack() )
                        {
                                cout << "DCClassPP ERROR (receive_update_broadcast_required_owner): Unpack failed for "
                                     << dcfield->get_name() << endl;
                        }
                }
        }

        di.skip_bytes( packer.get_num_unpacked_bytes() );
}

void DCClassPP::
receive_update_all_required( DistributedObjectBase *obj, DatagramIterator &di ) const
{
        DCPacker packer;
        const char *data = ( const char * )di.get_datagram().get_data();
        packer.set_unpack_data( data + di.get_current_index(),
                                di.get_remaining_size(), false );

        int num_fields = _dclass->get_num_inherited_fields();
        for ( int i = 0; i < num_fields; ++i )
        {
                DCField *dcfield = _dclass->get_inherited_field( i );
                DCFieldPP *field = get_field_wrapper( dcfield );
                if ( field == ( DCFieldPP * )NULL )
                {
                        continue;
                }
                if ( field->get_field()->as_molecular_field() == ( DCMolecularField * )NULL &&
                     field->get_field()->is_required() )
                {
                        packer.begin_unpack( field->get_field() );
                        packer.push();
                        ( *field->get_setter_func() )( packer, obj );
                        packer.pop();
                        if ( !packer.end_unpack() )
                        {
                                cout << "DCClassPP ERROR (receive_update_all_required): Unpack failed for "
                                     << dcfield->get_name() << endl;
                        }
                }
        }

        di.skip_bytes( packer.get_num_unpacked_bytes() );
}

void DCClassPP::
receive_update_other( DistributedObjectBase *obj, DatagramIterator &di ) const
{
        int num_fields = di.get_uint16();
        for ( int i = 0; i < num_fields; ++i )
        {
                receive_update( obj, di );
        }
}

void DCClassPP::
direct_update( DistributedObjectBase *obj, const string &field_name,
               const string &value_blob )
{
        DCFieldPP *field = get_field_wrapper( _dclass->get_field_by_name( field_name ) );
        nassertv_always( field != NULL );

        DCPacker packer;
        packer.set_unpack_data( value_blob );
        packer.begin_unpack( field->get_field() );
        packer.push();
        ( *field->get_setter_func() )( packer, obj );
        packer.pop();
        packer.end_unpack();
}

void DCClassPP::
direct_update( DistributedObjectBase *obj, const string &field_name, const Datagram &datagram )
{
        direct_update( obj, field_name, datagram.get_message() );
}

bool DCClassPP::
pack_required_field( Datagram &dg, DistributedObjectBase *obj, const DCFieldPP *field ) const
{
        DCPacker packer;
        packer.begin_pack( field->get_field() );
        packer.push();
        if ( !pack_required_field( packer, obj, field ) )
        {
                cout << "DCClassPP ERROR (pack_required_field): pack_required_field() failed for "
                     << field->get_field()->get_name() << endl;
                return false;
        }
        packer.pop();
        if ( !packer.end_pack() )
        {
                cout << "DCClassPP ERROR (pack_required_field): end_pack() failed for "
                     << field->get_field()->get_name() << endl;
                return false;
        }

        dg.append_data( packer.get_data(), packer.get_length() );
        return true;
}

bool DCClassPP::
pack_required_field( DCPacker &packer, DistributedObjectBase *obj,
                     const DCFieldPP *field ) const
{

        if ( field->get_field()->as_molecular_field() != ( DCMolecularField * )NULL )
        {
                cout << "DCClassPP ERROR (pack_required_field): " << field->get_field()->get_name()
                     << "is a molecular field." << endl;
                return false;
        }

        DCFunc *func = field->get_getter_func();
        if ( func == NULL )
        {
                cout << "DCClassPP ERROR (pack_required_field): Getter function for "
                     << field->get_field()->get_name() << " is undefined." << endl;
                return false;
        }

        // let the object pack the value in
        ( *func )( packer, obj );

        return true;
}

DCPacker DCClassPP::
start_client_format_update( const string &field_name, uint32_t do_id ) const
{
        DCFieldPP *field = get_field_wrapper( _dclass->get_field_by_name( field_name ) );
        if ( field == ( DCFieldPP * )NULL )
        {
                cerr << "No wrapper field named " << field_name << " in class " << _dclass->get_name() << endl;
                return DCPacker();
        }

        return field->start_client_format_update( do_id );
}

Datagram DCClassPP::
end_client_format_update( const string &field_name, DCPacker &pack ) const
{
        DCFieldPP *field = get_field_wrapper( _dclass->get_field_by_name( field_name ) );
        if ( field == ( DCFieldPP * )NULL )
        {
                cerr << "No wrapper field named " << field_name << " in class " << _dclass->get_name() << endl;
                return Datagram();
        }

        return field->end_client_format_update( pack );
}

DCPacker DCClassPP::
start_ai_format_update( const string &field_name, uint32_t do_id,
                        uint32_t to_id, uint32_t from_id ) const
{
        DCFieldPP *field = get_field_wrapper( _dclass->get_field_by_name( field_name ) );
        if ( field == ( DCFieldPP * )NULL )
        {
                cerr << "No wrapper for field named " << field_name << " in class " << _dclass->get_name() << endl;
                return DCPacker();
        }

        return field->start_ai_format_update( do_id, to_id, from_id );
}

Datagram DCClassPP::
end_ai_format_update( const string &field_name, DCPacker &pack ) const
{
        DCFieldPP *field = get_field_wrapper( _dclass->get_field_by_name( field_name ) );
        if ( field == ( DCFieldPP * )NULL )
        {
                cerr << "No wrapper for field named " << field_name << " in class " << _dclass->get_name() << endl;
                return Datagram();
        }

        return field->end_ai_format_update( pack );
}

DCPacker DCClassPP::
start_ai_format_update_msg_type( const string &field_name, uint32_t do_id,
                                 uint32_t to_id, uint32_t from_id, int msgtype ) const
{
        DCFieldPP *field = get_field_wrapper( _dclass->get_field_by_name( field_name ) );
        if ( field == ( DCFieldPP * )NULL )
        {
                cerr << "No wrapper for field named " << field_name << " in class " << _dclass->get_name() << endl;
                return DCPacker();
        }

        return field->start_ai_format_update_msg_type( do_id, to_id, from_id, msgtype );
}

Datagram DCClassPP::
end_ai_format_update_msg_type( const string &field_name, DCPacker &packer ) const
{
        DCFieldPP *field = get_field_wrapper( _dclass->get_field_by_name( field_name ) );
        if ( field == ( DCFieldPP * )NULL )
        {
                cerr << "No field named " << field_name << " in class " << _dclass->get_name() << endl;
                return Datagram();
        }

        return field->end_ai_format_update_msg_type( packer );
}

Datagram DCClassPP::
ai_format_generate( DistributedObjectBase *obj, DOID_TYPE do_id, DOID_TYPE parent_id,
                    ZONEID_TYPE zone_id, CHANNEL_TYPE dist_chan_id, CHANNEL_TYPE from_chan_id ) const
{
        DCPacker packer;

        packer.raw_pack_uint8( 1 );
        packer.RAW_PACK_CHANNEL( dist_chan_id );
        packer.RAW_PACK_CHANNEL( from_chan_id );

        packer.raw_pack_uint16( STATESERVER_CREATE_OBJECT_WITH_REQUIRED );

        packer.raw_pack_uint32( do_id );
        packer.raw_pack_uint32( parent_id );
        packer.raw_pack_uint32( zone_id );
        packer.raw_pack_uint16( _dclass->get_number() );

        // Now we're going to pack in the required fields.
        int num_fields = _dclass->get_num_inherited_fields();
        for ( int i = 0; i < num_fields; i++ )
        {
                DCField *dcfield = _dclass->get_inherited_field( i );
                DCFieldPP *field = get_field_wrapper( dcfield );
                if ( field == ( DCFieldPP * )NULL )
                {
                        continue;
                }
                if ( field->get_field()->is_required() && field->get_field()->as_molecular_field() == NULL )
                {
                        // This is a required field. It should have a getter defined.
                        packer.begin_pack( field->get_field() );
                        packer.push();
                        // Let's pack in the data from the getter of this field.
                        if ( !pack_required_field( packer, obj, field ) )
                        {
                                cout << "DCClassPP ERROR (ai_format_generate): pack_required_field() failed for "
                                     << field->get_field()->get_name() << endl;
                                return Datagram();
                        }
                        packer.pop();
                        packer.end_pack();
                }
        }

        return Datagram( packer.get_data(), packer.get_length() );
}

/**
* Find the wrapper of this DCField.
*/
DCFieldPP *DCClassPP::
get_field_wrapper( DCField *field ) const
{
        // Give us NULL? You get NULL.
        if ( field == NULL )
        {
                return NULL;
        }

        for ( size_t i = 0; i < _pp_fields.size(); i++ )
        {
                DCFieldPP *pfield = _pp_fields[i];
                if ( pfield->get_field()->get_number() == field->get_number() )
                {
                        return pfield;
                }
        }

        return NULL;
}

DCClass *DCClassPP::
get_dclass() const
{
        return _dclass;
}

void DCClassPP::
add_ppfield( DCFieldPP *ppfield )
{
        if ( find( _pp_fields.begin(), _pp_fields.end(), ppfield ) != _pp_fields.end() )
        {
                nassert_raise( "PPField " + ppfield->get_field()->get_name() + " already exists in PPClass " + _dclass->get_name() );
                return;
        }
        _pp_fields.push_back( ppfield );
}

Datagram DCClassPP::
client_format_generate_cmu( DistributedObjectBase *obj, DOID_TYPE do_id,
                            ZONEID_TYPE zone_id, pvector<string> &optional_fields )
{
        DCPacker packer;
        packer.raw_pack_uint16( CLIENT_OBJECT_GENERATE_CMU );

        packer.raw_pack_uint32( zone_id );
        packer.raw_pack_uint16( get_dclass()->get_number() );
        packer.raw_pack_uint32( do_id );

        int num_fields = get_dclass()->get_num_inherited_fields();
        for ( int i = 0; i < num_fields; i++ )
        {
                DCFieldPP *field = get_field_wrapper( get_dclass()->get_inherited_field( i ) );
                if ( field->get_field()->is_required() && field->get_field()->as_molecular_field() == NULL )
                {
                        packer.begin_pack( field->get_field() );
                        packer.push();
                        if ( !pack_required_field( packer, obj, field ) )
                        {
                                cout << "DCClassPP ERROR (client_format_generate_cmu): pack_required_field() failed for "
                                     << field->get_field()->get_name() << endl;
                                return Datagram();
                        }
                        packer.pop();
                        packer.end_pack();
                }
        }

        size_t num_optional_fields = optional_fields.size();
        for ( size_t i = 0; i < num_optional_fields; i++ )
        {
                string field_name = optional_fields[i];
                DCField *dc_field = get_dclass()->get_field_by_name( field_name );
                if ( dc_field == NULL )
                {
                        ostringstream ss;
                        ss << "No field named " << field_name << " in class " << get_dclass()->get_name() << "\n";
                        nassert_raise( ss.str() );
                        return Datagram();
                }
                DCFieldPP *field = get_field_wrapper( dc_field );
                packer.raw_pack_uint16( dc_field->get_number() );
                packer.begin_pack( dc_field );
                packer.push();
                if ( !pack_required_field( packer, obj, field ) )
                {
                        cout << "DCClassPP ERROR (client_format_generate_cmu): pack_required_field() failed for "
                             << field->get_field()->get_name() << endl;
                        return Datagram();
                }
                packer.pop();
                packer.end_pack();
        }

        return Datagram( packer.get_data(), packer.get_length() );
}