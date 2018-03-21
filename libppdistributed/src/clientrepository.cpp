#include "clientrepository.h"
#include "distributedobject.h"

#include <throw_event.h>
#include <uniqueIdAllocator.h>

NotifyCategoryDef( clientRepository, "" );

ClientRepository::
ClientRepository( vector<string> &dc_files, const string &dc_suffix, ConnectMethod cm ) :
        ClientRepositoryBase( dc_suffix, cm, dc_files ),
        _do_id_allocator( NULL )
{
        set_handle_datagrams_internally( false );

        _do_id_base = 0;
        _do_id_last = 0;

        _our_channel = 0;

        _current_sender_id = 0;

}

ClientRepository::
ClientRepository() :
        ClientRepositoryBase(),
        _do_id_allocator( NULL )
{
}

ClientRepository::
~ClientRepository()
{
        if ( _do_id_allocator != NULL )
        {
                delete _do_id_allocator;
        }
}

CHANNEL_TYPE ClientRepository::
get_our_channel() const
{
        return _our_channel;
}

void ClientRepository::
handle_set_doid_range( DatagramIterator &di )
{
        _do_id_base = di.get_uint32();
        _do_id_last = _do_id_base + di.get_uint32();
        clientRepository_cat.info()
                        << "Received DoId range from server (" << _do_id_base << " to " << _do_id_last - 1 << ")\n";

        if ( _do_id_allocator != NULL )
        {
                delete _do_id_allocator;
                _do_id_allocator = NULL;
        }

        _do_id_allocator = new UniqueIdAllocator( _do_id_base, _do_id_last - 1 );

        _our_channel = _do_id_base;

        create_ready();
}

void ClientRepository::
create_ready()
{
        throw_event( "createReady" );
        throw_event( PPUtils::unique_name( "createReady" ) );
}

void ClientRepository::
handle_request_generates( DatagramIterator &di )
{
        clientRepository_cat.debug()
                        << "handle_request_generates:\n";
        ZONEID_TYPE zone = di.get_uint32();
        clientRepository_cat.debug()
                        << "/tZone: " << zone << "\n";
        for ( DoCollectionManager::DoMap::iterator itr = _do_id_2_do.begin(); itr != _do_id_2_do.end(); ++itr )
        {
                DistributedObjectBase *dist_obj = itr->second;
                if ( dist_obj->get_zone_id() == zone )
                {
                        if ( is_local_id( dist_obj->get_do_id() ) )
                        {
                                clientRepository_cat.debug()
                                                << "Resending generate...\n";
                                resend_generate( dist_obj );
                        }
                }
        }
}

void ClientRepository::
resend_generate( DistributedObjectBase *dist_obj )
{
        pvector<string> extra_fields;
        for ( int i = 0; i < dist_obj->get_dclass()->get_dclass()->get_num_inherited_fields(); i++ )
        {
                DCFieldPP *field = dist_obj->get_dclass()->get_field_wrapper(
                                           dist_obj->get_dclass()->get_dclass()->get_inherited_field( i ) );
                if ( field->get_field()->has_keyword( "broadcast" ) && ( field->get_field()->has_keyword( "required" ) ) )
                {
                        if ( field->get_field()->as_molecular_field() != NULL )
                        {
                                continue;
                        }
                        extra_fields.push_back( field->get_field()->get_name() );
                }
        }

        Datagram dg = dist_obj->get_dclass()->client_format_generate_cmu(
                              dist_obj, dist_obj->get_do_id(), dist_obj->get_zone_id(), extra_fields );
        send( dg );
}

void ClientRepository::
handle_generate( DatagramIterator &di )
{
        _current_sender_id = di.get_uint32();
        ZONEID_TYPE zone_id = di.get_uint32();
        uint16_t class_id = di.get_uint16();
        DOID_TYPE do_id = di.get_uint32();

        // Look up the dclass.
        DCClassPP *dclass = _dclass_by_number[class_id];

        DistributedObjectBase *dist_obj;
        if ( _do_id_2_do.find( do_id ) != _do_id_2_do.end() )
        {
                dist_obj = _do_id_2_do[do_id];
                if ( dist_obj->get_dclass() == dclass )
                {
                        // We've already got this object. Probably this is just a
                        // repeat-generate, synthesized for the benefit of someone
                        // else who just entered the zone. Accept the new updates,
                        // but don't make a formal generate.
                        dclass->receive_update_broadcast_required( dist_obj, di );
                        dclass->receive_update_other( dist_obj, di );
                        return;
                }
        }


        dclass->get_dclass()->start_generate();
        // Create a new distributed object, and put it in the dictionary.
        dist_obj = generate_with_required_other_fields( dclass, do_id, di, 0, zone_id );
        dclass->get_dclass()->stop_generate();
}

DOID_TYPE ClientRepository::
allocate_do_id()
{
        clientRepository_cat.debug()
                        << "Allocating do_id\n" << endl;
        nassertr( _do_id_allocator != NULL, 0 );
        return _do_id_allocator->allocate();
}

DOID_TYPE ClientRepository::
reserve_do_id( DOID_TYPE do_id )
{
        nassertr( _do_id_allocator != NULL, do_id );
        _do_id_allocator->initial_reserve_id( do_id );
        return do_id;
}

void ClientRepository::
free_do_id( DOID_TYPE do_id )
{
        nassertv( is_local_id( do_id ) );
        nassertv( _do_id_allocator != NULL );
        _do_id_allocator->free( do_id );
}

void ClientRepository::
store_object_location( DistributedObjectBase *dist_obj, DOID_TYPE parent_id, ZONEID_TYPE zone_id )
{
        dist_obj->set_parent_id( parent_id );
        dist_obj->set_zone_id( zone_id );
}

DistributedObjectBase *ClientRepository::
create_distributed_object( DistributedObjectBase *obj, ZONEID_TYPE zone_id, bool explicit_do_id, DOID_TYPE do_id, DCClassPP *dclass )
{
        if ( !explicit_do_id )
        {
                do_id = allocate_do_id();
        }

        if ( dclass == NULL )
        {
                string type_name = obj->get_type().get_name();
                if ( _dclass_by_name.find( type_name ) == _dclass_by_name.end() )
                {
                        stringstream ss;
                        ss
                                        << "No Dclass exists with name " << type_name
                                        << ". If this is an ownerview object, make "
                                        << "sure to pass the dclass of the parent class. "
                                        << "For example: A LocalToon's dclass should be DistributedToon.\n";
                        nassert_raise( ss.str() );
                        return obj;
                }
                dclass = _dclass_by_name[obj->get_type().get_name()];
        }

        obj->set_dclass( dclass );
        obj->set_do_id( do_id );
        _do_id_2_do[do_id] = obj;
        obj->generate_init();
        obj->generate();
        obj->set_location( 0, zone_id );
        obj->announce_generate();
        Datagram dg = obj->get_dclass()->client_format_generate_cmu( obj, do_id, zone_id );
        send( dg );

        return obj;
}

void ClientRepository::
send_delete_msg( DOID_TYPE do_id )
{
        Datagram dg;
        dg.add_uint16( OBJECT_DELETE_CMU );
        dg.add_uint32( do_id );
        send( dg );
}

void ClientRepository::
send_disconnect()
{
        if ( is_connected() )
        {
                Datagram dg;
                dg.add_uint16( CLIENT_DISCONNECT_CMU );
                send( dg );
                clientRepository_cat.info()
                                << "Sent disconnect message to server\n";
                disconnect();
        }
        stop_heartbeat();
}

void ClientRepository::
set_interest_zones( vector<ZONEID_TYPE> &interest_zone_ids )
{
        Datagram dg;
        dg.add_uint16( CLIENT_SET_INTEREST_CMU );
        for ( size_t i = 0; i < interest_zone_ids.size(); i++ )
        {
                dg.add_uint32( interest_zone_ids[i] );
        }

        send( dg );
        _interest_zones = interest_zone_ids;
}

void ClientRepository::
set_object_zone( DistributedObject *dist_obj, ZONEID_TYPE zone_id )
{
        dist_obj->b_set_location( 0, zone_id );
        nassertv( dist_obj->get_zone_id() == zone_id );

        resend_generate( dist_obj );
}

void ClientRepository::
send_set_location( DOID_TYPE do_id, DOID_TYPE parent_id, ZONEID_TYPE zone_id )
{
        Datagram dg;
        dg.add_uint16( OBJECT_SET_ZONE_CMU );
        dg.add_uint32( do_id );
        dg.add_uint32( zone_id );
        send( dg );
}

void ClientRepository::
send_heartbeat()
{
        Datagram dg;
        dg.add_uint16( CLIENT_HEARTBEAT_CMU );
        send( dg );

}

bool ClientRepository::
is_local_id( DOID_TYPE do_id ) const
{
        return ( ( do_id >= _do_id_base ) && ( do_id < _do_id_last ) );
}

bool ClientRepository::
have_create_authority() const
{
        return ( _do_id_last > _do_id_base );
}

DOID_TYPE ClientRepository::
get_avatar_id_from_sender() const
{
        return _current_sender_id;
}

void ClientRepository::
handle_datagram( DatagramIterator &di )
{
        uint16_t msg_type = get_msg_type();
        _current_sender_id = 0;

        switch ( msg_type )
        {
        case SET_DOID_RANGE_CMU:
                handle_set_doid_range( di );
                break;
        case OBJECT_GENERATE_CMU:
                handle_generate( di );
                break;
        case OBJECT_UPDATE_FIELD_CMU:
                handle_update_field( di );
                break;
        case OBJECT_DISABLE_CMU:
                handle_disable( di );
                break;
        case OBJECT_DELETE_CMU:
                handle_delete( di );
                break;
        case REQUEST_GENERATES_CMU:
                handle_request_generates( di );
                break;
        default:
                handle_message_type( msg_type, di );
                break;
        }

        consider_heartbeat();
}

void ClientRepository::
handle_update_field( DatagramIterator &di )
{
        _current_sender_id = di.get_uint32();
        clientRepository_cat.debug()
                        << "update_field from " << _current_sender_id << "\n";
        ClientRepositoryBase::handle_update_field( di );
}

void ClientRepository::
handle_delete( DatagramIterator &di )
{
        DOID_TYPE do_id = di.get_uint32();
        delete_object( do_id );
}

void ClientRepository::
handle_disable( DatagramIterator &di )
{
        while ( di.get_remaining_size() > 0 )
        {
                DOID_TYPE do_id = di.get_uint32();
                nassertv( !is_local_id( do_id ) );
                disable_do_id( do_id );
        }
}

void ClientRepository::
delete_object( DOID_TYPE do_id )
{
        if ( _do_id_2_do.find( do_id ) != _do_id_2_do.end() )
        {
                DistributedObjectBase *pObj = _do_id_2_do[do_id];
                _do_id_2_do.erase( _do_id_2_do.find( do_id ) );
                pObj->delete_or_delay();
                if ( is_local_id( do_id ) )
                {
                        free_do_id( do_id );
                }
        }
        else
        {
                clientRepository_cat.warning()
                                << "Asked to delete non-existent DistObj " << do_id << "\n";
        }
}

void ClientRepository::
handle_message_type( uint16_t msg_type, DatagramIterator &di )
{
        clientRepository_cat.error()
                        << "unrecognized message type " << msg_type << "\n";
}