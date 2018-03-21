#include "server_repository.h"
#include "pp_globals.h"

#include <connection.h>
#include <config_pandaplus.h>

NotifyCategoryDef( serverRepository, "" );

ServerRepository::
ServerRepository( int tcp_port, const string &server_address, stringvec_t &dc_files ) :
        _qcl( &_qcm, 0 ),
        _qcr( &_qcm, 0 ),
        _cw( &_qcm, 0 ),
        _do_id_range( "server-doid-range", 1000000 ),
        _address( server_address ),
        _port( tcp_port ),
        _dc_files( dc_files )
{
        _id_alloc = new UniqueIdAllocator( 0, 0xffffffff / _do_id_range );
        _dc_suffix = "";
}

void ServerRepository::
start_server()
{
        _tcp_rendezvous = _qcm.open_TCP_server_rendezvous( _address, _port, 10 );
        _qcl.add_connection( _tcp_rendezvous );

        _listener_poll_task = g_base->start_task( listener_poll, this, "serverListenerPollTask" );
        _reader_poll_task = g_base->start_task( reader_poll, this, "serverReaderPollTask" );
        _client_dc_task = g_base->start_task( client_dc_task, this, "client_dc_task" );

        read_dc_file( _dc_files );
}

ServerRepository::
~ServerRepository()
{
}

void ServerRepository::
set_tcp_header_size( int header_size )
{
        _qcr.set_tcp_header_size( header_size );
        _cw.set_tcp_header_size( header_size );
}

int ServerRepository::
get_tcp_header_size() const
{
        return _qcr.get_tcp_header_size();
}

void ServerRepository::
read_dc_file( stringvec_t &dc_file_names )
{
        _dc_file.clear();
        _dclasses_by_name.clear();
        _dclasses_by_number.clear();
        _hash_val = 0;

        for ( size_t i = 0; i < dc_file_names.size(); i++ )
        {
                serverRepository_cat.info()
                                << "Reading DC file: " << dc_file_names[i] << "...\n";
                bool result = _dc_file.read( Filename( dc_file_names[i] ) );
                if ( !result )
                {
                        serverRepository_cat.error()
                                        << "Could not read dc file!\n";
                        return;
                }
        }

        _hash_val = _dc_file.get_hash();

        for ( int j = 0; j < _dc_file.get_num_classes(); j++ )
        {
                DCClass *dclass = _dc_file.get_class( j );
                int class_num = dclass->get_number();
                string class_name = dclass->get_name();

                DCClassPP *cls_wrapper = new DCClassPP( dclass );

                _dclasses_by_name[class_name] = cls_wrapper;
                if ( class_num >= 0 )
                {
                        _dclasses_by_number[class_num] = cls_wrapper;
                }

                for ( int k = 0; k < dclass->get_num_inherited_fields(); k++ )
                {
                        DCField *field = dclass->get_inherited_field( k );
                        DCFieldPP *field_wrapper = NULL;
                        if ( find( _wrapped_fields.begin(), _wrapped_fields.end(), field ) == _wrapped_fields.end() )
                        {
                                field_wrapper = new DCFieldPP( field->as_atomic_field() );
                                _wrapped_fields.push_back( field );
                                _wrapper_fields.push_back( field_wrapper );
                        }
                        else
                        {
                                field_wrapper = get_field_wrapper( field );
                        }

                        cls_wrapper->add_ppfield( field_wrapper );
                }
        }
}

DCFieldPP *ServerRepository::
get_field_wrapper( DCField *field )
{
        for ( size_t i = 0; i < _wrapper_fields.size(); i++ )
        {
                DCFieldPP *wrapper_field = _wrapper_fields[i];
                if ( wrapper_field->get_field()->get_number() == field->get_number() )
                {
                        return wrapper_field;
                }
        }

        return ( DCFieldPP * )NULL;
}

AsyncTask::DoneStatus ServerRepository::
listener_poll( GenericAsyncTask *task, void *data )
{
        ServerRepository *sr = ( ServerRepository * )data;
        if ( sr->_qcl.new_connection_available() )
        {
                PT( Connection ) rendezvous;
                NetAddress net_address;
                PT( Connection ) new_conn;
                bool ret_val = sr->_qcl.get_new_connection( rendezvous, net_address, new_conn );
                if ( !ret_val )
                {
                        return AsyncTask::DS_cont;
                }

                new_conn = new_conn.p();

                DOID_TYPE id = sr->_id_alloc->allocate();
                DOID_TYPE do_id_base = id * sr->_do_id_range + 1;

                serverRepository_cat.info()
                                << "Got client " << do_id_base << " from " << net_address << "\n";

                Client *client = new Client;
                client->connection = new_conn;
                client->net_address = net_address;
                client->do_id_base = do_id_base;

                sr->_clients_by_connection[client->connection] = client;
                sr->_clients_by_do_id_base[client->do_id_base] = client;

                sr->_qcr.add_connection( new_conn );

                sr->send_do_id_range( client );
        }
        return AsyncTask::DS_cont;
}

AsyncTask::DoneStatus ServerRepository::
reader_poll( GenericAsyncTask *task, void *data )
{
        ServerRepository *sr = ( ServerRepository * )data;
        sr->reader_poll_once();
        return AsyncTask::DS_cont;
}

bool ServerRepository::
reader_poll_once()
{
        bool avail_get = _qcr.data_available();
        if ( avail_get )
        {
                NetDatagram datagram;
                bool read_ret = _qcr.get_data( datagram );
                if ( read_ret )
                {
                        handle_datagram( datagram );
                }
        }
        return avail_get;
}

void ServerRepository::
handle_datagram( NetDatagram &datagram )
{
        PT( Connection ) connection = datagram.get_connection();

        // Make sure that this connection exists in our connection map.
        if ( _clients_by_connection.find( connection ) == _clients_by_connection.end() )
        {
                // It doesn't, strange.
                serverRepository_cat.warning()
                                << "Ignoring datagram from unknown connection " <<
                                datagram.get_connection() << "\n";
                return;
        }

        Client *client = _clients_by_connection[datagram.get_connection()];

        DatagramIterator dgi( datagram );
        uint16_t msg_type = dgi.get_uint16();

        serverRepository_cat.debug()
                        << "Received datagram with msgType " << msg_type << "\n";

        switch ( msg_type )
        {
        case CLIENT_DISCONNECT_CMU:
                handle_client_disconnect( client );
                break;
        case CLIENT_SET_INTEREST_CMU:
                handle_client_set_interest( client, dgi );
                break;
        case CLIENT_OBJECT_GENERATE_CMU:
                handle_client_create_object( datagram, dgi );
                break;
        case CLIENT_OBJECT_SET_FIELD:
                handle_client_object_update_field( datagram, dgi );
                break;
        case CLIENT_OBJECT_UPDATE_FIELD_TARGETED_CMU:
                handle_client_object_update_field( datagram, dgi, true );
                break;
        case OBJECT_DELETE_CMU:
                handle_client_delete_object( datagram, dgi.get_uint32() );
                break;
        case OBJECT_SET_ZONE_CMU:
                handle_client_object_set_zone( datagram, dgi );
                break;
        case CLIENT_HEARTBEAT_CMU:
                // We don't care about heartbeats.
                break;
        default:
                handle_message_type( msg_type, dgi );
                break;
        }
}

void ServerRepository::
handle_message_type( uint16_t msg_type, DatagramIterator &dgi )
{
        serverRepository_cat.warning()
                        << "unrecognized message type " << msg_type << "\n";
}

void ServerRepository::
handle_client_create_object( NetDatagram &datagram, DatagramIterator &dgi )
{
        serverRepository_cat.debug()
                        << "handle_client_create_object:\n";

        PT( Connection ) connection = datagram.get_connection();
        ZONEID_TYPE zone_id = dgi.get_uint32();
        uint16_t class_id = dgi.get_uint16();
        DOID_TYPE do_id = dgi.get_uint32();

        serverRepository_cat.debug()
                        << "\tzoneId: " << zone_id << "\n";
        serverRepository_cat.debug()
                        << "\tclassId: " << class_id << "\n";
        serverRepository_cat.debug()
                        << "\tdoId: " << do_id << "\n";

        Client *client = _clients_by_connection[connection];

        if ( get_do_id_base( do_id ) != client->do_id_base )
        {
                serverRepository_cat.warning()
                                << "Ignoring attempt to create invalid do_id " << do_id <<
                                " from client " << client->do_id_base << "\n";
                return;
        }

        DCClassPP *dclass = _dclasses_by_number[class_id];

        serverRepository_cat.debug()
                        << "\tDClass: " << dclass->get_dclass()->get_name() << "\n";

        Object *obj;
        if ( client->objects_by_doid.find( do_id ) != client->objects_by_doid.end() )
        {
                obj = client->objects_by_doid[do_id];
                if ( obj->dclass != dclass )
                {
                        serverRepository_cat.warning()
                                        << "Ignoring attempt to change object " << do_id << " from " <<
                                        obj->dclass->get_dclass()->get_name() << " to " <<
                                        dclass->get_dclass()->get_name() << " by client " <<
                                        client->do_id_base << "\n";
                        return;
                }
                set_object_zone( client, obj, zone_id );
        }
        else
        {
                obj = new Object;
                obj->do_id = do_id;
                obj->zone_id = zone_id;
                obj->dclass = dclass;

                client->objects_by_doid[do_id] = obj;

                client->objects_by_zoneid[zone_id].push_back( obj );
                _objects_by_zone_id[zone_id].push_back( obj );

                update_client_interest_zones( client );
        }

        NetDatagram dg;
        dg.add_uint16( OBJECT_GENERATE_CMU );
        dg.add_uint32( client->do_id_base );
        dg.add_uint32( zone_id );
        dg.add_uint16( class_id );
        dg.add_uint32( do_id );
        dg.append_data( dgi.get_remaining_bytes() );

        ClientVec exp_list;
        exp_list.push_back( client );
        send_to_zone_except( zone_id, dg, exp_list );
}

static void unknown_object_update( DOID_TYPE do_id, DOID_TYPE do_id_base )
{
        serverRepository_cat.warning()
                        << "Ignoring update for unknown object " << do_id << " from client " << do_id_base << "\n";
}

void ServerRepository::
handle_client_object_update_field( NetDatagram &datagram, DatagramIterator &dgi, bool targeted )
{
        serverRepository_cat.debug()
                        << "handle_client_object_update_field:\n";

        PT( Connection ) connection = datagram.get_connection();
        Client *client = _clients_by_connection[connection];

        DOID_TYPE target_id;
        if ( targeted )
        {
                serverRepository_cat.debug()
                                << "    targeted: " << targeted << "\n";
                target_id = dgi.get_uint32();
                serverRepository_cat.debug()
                                << "    target_id: " << target_id << "\n";
        }

        DOID_TYPE do_id = dgi.get_uint32();
        serverRepository_cat.debug()
                        << "    do_id: " << do_id << "\n";
        uint16_t field_id = dgi.get_uint16();
        serverRepository_cat.debug()
                        << "    field_id: " << field_id << "\n";

        DOID_TYPE do_id_base = get_do_id_base( do_id );
        serverRepository_cat.debug()
                        << "    do_id_base: " << do_id_base << "\n";
        Client *owner = NULL;
        Object *object = NULL;
        if ( _clients_by_do_id_base.find( do_id_base ) != _clients_by_do_id_base.end() )
        {
                owner = _clients_by_do_id_base[do_id_base];
        }
        else
        {
                serverRepository_cat.warning()
                                << "Unable to find client with do_id_base " << do_id_base << "\n";
                unknown_object_update( do_id, do_id_base );
                return;
        }
        if ( owner == NULL || owner->objects_by_doid.find( do_id ) == owner->objects_by_doid.end() )
        {
                unknown_object_update( do_id, do_id_base );
                return;
        }
        else
        {
                object = owner->objects_by_doid[do_id];
        }

        DCFieldPP *dc_field = object->dclass->get_field_wrapper(
                                      object->dclass->get_dclass()->get_field_by_index( field_id ) );
        if ( dc_field == NULL )
        {
                serverRepository_cat.warning()
                                << "Ignoring update for field " << field_id << " on object " <<
                                do_id << " from client " << client->do_id_base << "; no such field for class " <<
                                object->dclass->get_dclass()->get_name() << ".\n";
                return;
        }

        if ( client != owner )
        {
                if ( !dc_field->get_field()->has_keyword( "clsend" ) &&
                     !dc_field->get_field()->has_keyword( "p2p" ) )
                {
                        serverRepository_cat.warning()
                                        << "Ignoring update for " << object->dclass->get_dclass()->get_name() <<
                                        "." << dc_field->get_field()->get_name() << " on object " << do_id <<
                                        " from client " << client->do_id_base << ": not owner\n";
                        return;
                }
        }

        NetDatagram dg;
        dg.add_uint16( OBJECT_UPDATE_FIELD_CMU );
        dg.add_uint32( client->do_id_base );
        dg.add_uint32( do_id );
        dg.add_uint16( field_id );
        dg.append_data( dgi.get_remaining_bytes() );

        if ( targeted )
        {
                Client *target;
                if ( _clients_by_do_id_base.find( target_id ) == _clients_by_do_id_base.end() )
                {
                        serverRepository_cat.warning()
                                        << "Ignoring targeted update to " << target_id << " for " <<
                                        object->dclass->get_dclass()->get_name() << "." <<
                                        dc_field->get_field()->get_name() << " on object " << do_id <<
                                        " from client " << client->do_id_base << ": target not known\n";
                        return;
                }
                else
                {
                        target = _clients_by_do_id_base[target_id];
                }
                _cw.send( dg, target->connection );
        }
        else if ( dc_field->get_field()->has_keyword( "p2p" ) )
        {
                _cw.send( dg, owner->connection );
        }
        else if ( dc_field->get_field()->has_keyword( "broadcast" ) )
        {
                ClientVec exp_list;
                exp_list.push_back( client );
                send_to_zone_except( object->zone_id, dg, exp_list );
        }
        else if ( dc_field->get_field()->has_keyword( "reflect" ) )
        {
                ClientVec exp_list;
                send_to_zone_except( object->zone_id, dg, exp_list );
        }
        else
        {
                serverRepository_cat.warning()
                                << "Message is not broadcast or p2p\n";
        }
}

DOID_TYPE ServerRepository::
get_do_id_base( DOID_TYPE do_id )
{
        return ( ( int )( do_id / _do_id_range ) ) * _do_id_range + 1;
}

void ServerRepository::
handle_client_delete_object( NetDatagram &datagram, DOID_TYPE do_id )
{
        serverRepository_cat.debug()
                        << "handle_client_delete_object:\n";
        serverRepository_cat.debug()
                        << "\tdoId: " << do_id << "\n";

        PT( Connection ) connection = datagram.get_connection();
        Client *client = _clients_by_connection[connection];
        Object *object = NULL;
        if ( client->objects_by_doid.find( do_id ) == client->objects_by_doid.end() )
        {
                serverRepository_cat.warning()
                                << "Ignoring update for unkown object " << do_id << " from client "
                                << client->do_id_base << "\n";
                return;
        }
        else
        {
                object = client->objects_by_doid[do_id];
        }

        ClientVec exp_list;
        send_to_zone_except( object->zone_id, datagram, exp_list );

        _objects_by_zone_id[object->zone_id].erase( find( _objects_by_zone_id[object->zone_id].begin(),
                                                          _objects_by_zone_id[object->zone_id].end(), object ) );
        if ( _objects_by_zone_id[object->zone_id].size() == ( size_t )0 )
        {
                _objects_by_zone_id.erase( _objects_by_zone_id.find( object->zone_id ) );
        }

        client->objects_by_zoneid[object->zone_id].erase( find( client->objects_by_zoneid[object->zone_id].begin(),
                                                                client->objects_by_zoneid[object->zone_id].end(), object ) );
        if ( client->objects_by_zoneid[object->zone_id].size() == ( size_t )0 )
        {
                client->objects_by_zoneid.erase( client->objects_by_zoneid.find( object->zone_id ) );
        }

        client->objects_by_doid.erase( client->objects_by_doid.find( do_id ) );

        update_client_interest_zones( client );

        // Free the memory allocated for this object struct.
        delete object;
}

void ServerRepository::
handle_client_object_set_zone( NetDatagram &datagram, DatagramIterator &dgi )
{
        serverRepository_cat.debug()
                        << "handle_client_object_set_zone:\n";
        DOID_TYPE do_id = dgi.get_uint32();
        ZONEID_TYPE zone_id = dgi.get_uint32();

        serverRepository_cat.debug()
                        << "\tdoId: " << do_id << "\n";
        serverRepository_cat.debug()
                        << "\tzoneId: " << zone_id << "\n";

        PT( Connection ) connection = datagram.get_connection();
        Client *client = _clients_by_connection[connection];
        Object *object = NULL;
        if ( client->objects_by_doid.find( do_id ) == client->objects_by_doid.end() )
        {
                serverRepository_cat.warning()
                                << "Ignoring object location for " << do_id << ": unknown\n";
                return;
        }
        else
        {
                object = client->objects_by_doid[do_id];
        }

        set_object_zone( client, object, zone_id );
}

void ServerRepository::
set_object_zone( Client *owner, Object *object, ZONEID_TYPE zone_id )
{
        if ( object->zone_id == zone_id )
        {
                // No change.
                return;
        }

        ZONEID_TYPE old_zone_id = object->zone_id;
        _objects_by_zone_id[object->zone_id].erase( find( _objects_by_zone_id[object->zone_id].begin(),
                                                          _objects_by_zone_id[object->zone_id].end(),
                                                          object ) );
        if ( _objects_by_zone_id[object->zone_id].size() == ( size_t )0 )
        {
                _objects_by_zone_id.erase( _objects_by_zone_id.find( object->zone_id ) );
        }

        owner->objects_by_zoneid[object->zone_id].erase( find( owner->objects_by_zoneid[object->zone_id].begin(),
                                                               owner->objects_by_zoneid[object->zone_id].end(),
                                                               object ) );
        if ( owner->objects_by_zoneid[object->zone_id].size() == ( size_t )0 )
        {
                owner->objects_by_zoneid.erase( owner->objects_by_zoneid.find( object->zone_id ) );
        }

        object->zone_id = zone_id;
        _objects_by_zone_id[zone_id].push_back( object );
        owner->objects_by_zoneid[zone_id].push_back( object );

        update_client_interest_zones( owner );

        NetDatagram dg;
        dg.add_uint16( OBJECT_DISABLE_CMU );
        dg.add_uint32( object->do_id );
        for ( size_t i = 0; i < _zones_to_clients[old_zone_id].size(); i++ )
        {
                Client *client = _zones_to_clients[old_zone_id][i];
                if ( client != owner )
                {
                        if ( find( client->current_interest_zone_ids.begin(),
                                   client->current_interest_zone_ids.end(),
                                   zone_id ) == client->current_interest_zone_ids.end() )
                        {
                                _cw.send( dg, client->connection );
                        }
                }
        }

}

void ServerRepository::
send_do_id_range( Client *client )
{
        NetDatagram datagram;
        datagram.add_uint16( SET_DOID_RANGE_CMU );
        datagram.add_uint32( client->do_id_base );
        datagram.add_uint32( _do_id_range );

        _cw.send( datagram, client->connection );
}

void ServerRepository::handle_client_disconnect( Client *client )
{
        serverRepository_cat.debug()
                        << "handle_client_disconnect:\n";
        serverRepository_cat.debug()
                        << "\tdoIdBase: " << client->do_id_base << "\n";
        serverRepository_cat.debug()
                        << "\townedObjects: " << client->objects_by_doid.size() << "\n";
        serverRepository_cat.debug()
                        << "\tinterestZones: " << client->current_interest_zone_ids.size() << endl;
        for ( size_t i = 0; i < client->current_interest_zone_ids.size(); i++ )
        {
                ZONEID_TYPE zone_id = client->current_interest_zone_ids[i];
                if ( _zones_to_clients[zone_id].size() == ( size_t )1 )
                {
                        serverRepository_cat.debug()
                                        << "Deleting zone id" << zone_id << "\n";
                        _zones_to_clients.erase( _zones_to_clients.find( zone_id ) );
                }
                else
                {
                        serverRepository_cat.debug()
                                        << "Deleting the client from zone " << zone_id << "\n";
                        _zones_to_clients[zone_id].erase( find( _zones_to_clients[zone_id].begin(),
                                                                _zones_to_clients[zone_id].end(),
                                                                client ) );
                }
        }

        for ( ObjectsByDoId::iterator itr = client->objects_by_doid.begin(); itr != client->objects_by_doid.end(); ++itr )
        {
                // Delete all objects associated with the client that has disconnected.

                Object *object = itr->second;
                serverRepository_cat.debug()
                                << "Creating object_delete_cmu datagram\n";
                serverRepository_cat.debug()
                                << "do_id: " << object->do_id << "\n";
                NetDatagram dg;
                dg.add_uint16( OBJECT_DELETE_CMU );
                dg.add_uint32( object->do_id );
                serverRepository_cat.debug()
                                << "Sending it out!" << endl;
                send_to_zone_except( object->zone_id, dg, ClientVec() );

                serverRepository_cat.debug()
                                << "Removing object from zone " << object->zone_id << "\n";
                _objects_by_zone_id[object->zone_id].erase( find( _objects_by_zone_id[object->zone_id].begin(),
                                                                  _objects_by_zone_id[object->zone_id].end(),
                                                                  object ) );
                serverRepository_cat.debug()
                                << "done\n";
                if ( _objects_by_zone_id[object->zone_id].size() == ( size_t )0 )
                {
                        serverRepository_cat.debug()
                                        << "Removing object zone " << object->zone_id << "\n";
                        _objects_by_zone_id.erase( _objects_by_zone_id.find( object->zone_id ) );
                }

                serverRepository_cat.debug()
                                << "Deleting object struct!\n";

                // Free the memory allocated for this object struct.
                delete object;
        }

        serverRepository_cat.debug()
                        << "Clearing client object maps...\n";
        client->objects_by_doid.clear();
        client->objects_by_zoneid.clear();

        serverRepository_cat.debug()
                        << "Removing client from connection and doidbase maps...\n";
        _clients_by_connection.erase( _clients_by_connection.find( client->connection ) );
        _clients_by_do_id_base.erase( _clients_by_do_id_base.find( client->do_id_base ) );

        uint32_t id = client->do_id_base / _do_id_range;
        serverRepository_cat.debug()
                        << "Freeing id " << id << "\n";
        _id_alloc->free( id );

        serverRepository_cat.debug()
                        << "Removing connection of reader and manager...\n";
        _qcr.remove_connection( client->connection );
        _qcm.close_connection( client->connection );

        on_client_disconnect( client->do_id_base );

        serverRepository_cat.debug()
                        << "Deleting client struct.\n";
        // Free the memory allocated for this client struct.
        delete client;
}

void ServerRepository::
on_client_disconnect( DOID_TYPE do_id_base )
{
        // If you want to do something when a client disconnnects, override
        // this function.
}

void ServerRepository::
handle_client_set_interest( Client *client, DatagramIterator &dgi )
{
        serverRepository_cat.debug()
                        << "handle_client_set_interest:\n";

        stringstream ss;
        vector<ZONEID_TYPE> zone_ids;
        while ( dgi.get_remaining_size() > 0 )
        {
                ZONEID_TYPE zone_id = dgi.get_uint32();
                ss << zone_id;
                if ( dgi.get_remaining_size() > 0 )
                {
                        ss << ", ";
                }
                zone_ids.push_back( zone_id );
        }

        serverRepository_cat.debug()
                        << "\tinterestZones: " << ss.str() << "\n";

        client->explicit_interest_zone_ids = zone_ids;
        update_client_interest_zones( client );
}

void ServerRepository::
update_client_interest_zones( Client *client )
{
        vector<ZONEID_TYPE> orig_zone_ids = client->current_interest_zone_ids;
        vector<ZONEID_TYPE> new_zone_ids = vector<ZONEID_TYPE>( client->explicit_interest_zone_ids );
        for ( ObjectsByZoneId::iterator itr = client->objects_by_zoneid.begin(); itr != client->objects_by_zoneid.end(); ++itr )
        {
                ZONEID_TYPE zone_id = itr->first;
                // Avoid duplicate zones.
                if ( find( new_zone_ids.begin(), new_zone_ids.end(), zone_id ) == new_zone_ids.end() )
                {
                        new_zone_ids.push_back( zone_id );
                }
        }
        if ( orig_zone_ids == new_zone_ids )
        {
                // No change.
                serverRepository_cat.debug()
                                << "No change in interests zones\n" << endl;
                return;
        }

        client->current_interest_zone_ids = new_zone_ids;
        vector<ZONEID_TYPE> added_zone_ids;
        for ( size_t i = 0; i < new_zone_ids.size(); i++ )
        {
                ZONEID_TYPE new_zone = new_zone_ids[i];
                if ( find( orig_zone_ids.begin(), orig_zone_ids.end(), new_zone ) == orig_zone_ids.end() )
                {
                        serverRepository_cat.debug()
                                        << "Found new zone: " << new_zone << "\n";
                        added_zone_ids.push_back( new_zone );
                }
        }
        vector<ZONEID_TYPE> removed_zone_ids;
        for ( size_t j = 0; j < orig_zone_ids.size(); j++ )
        {
                ZONEID_TYPE old_zone = orig_zone_ids[j];
                if ( find( new_zone_ids.begin(), new_zone_ids.end(), old_zone ) == new_zone_ids.end() )
                {
                        serverRepository_cat.debug()
                                        << "Found old zone: " << old_zone << "\n";
                        removed_zone_ids.push_back( old_zone );
                }
        }

        for ( size_t k = 0; k < added_zone_ids.size(); k++ )
        {
                ZONEID_TYPE zone_id = added_zone_ids[k];
                ClientVec clients;
                clients.push_back( client );
                _zones_to_clients[zone_id].push_back( client );

                serverRepository_cat.debug()
                                << "Sending request generates\n" << endl;

                NetDatagram dg;
                dg.add_uint16( REQUEST_GENERATES_CMU );
                dg.add_uint32( zone_id );
                send_to_zone_except( zone_id, dg, clients );
        }

        Datagram dg;
        dg.add_uint16( OBJECT_DISABLE_CMU );
        for ( size_t l = 0; l < removed_zone_ids.size(); l++ )
        {
                ZONEID_TYPE zone_id = removed_zone_ids[l];
                _zones_to_clients[zone_id].erase( find( _zones_to_clients[zone_id].begin(),
                                                        _zones_to_clients[zone_id].end(),
                                                        client ) );

                if ( _objects_by_zone_id.find( zone_id ) != _objects_by_zone_id.end() )
                {
                        ObjectVec object_vec = _objects_by_zone_id[zone_id];
                        for ( size_t m = 0; m < object_vec.size(); m++ )
                        {
                                Object *object = object_vec[m];
                                serverRepository_cat.debug()
                                                << "Adding do_id " << object->do_id << " to object_disable_cmu list.\n" << endl;
                                dg.add_uint32( object->do_id );
                        }
                }
        }
        serverRepository_cat.debug()
                        << "Sending OBJECT_DISABLE_CMU\n" << endl;
        _cw.send( dg, client->connection );
}

AsyncTask::DoneStatus ServerRepository::
client_dc_task( GenericAsyncTask *task, void *data )
{
        ServerRepository *sr = ( ServerRepository * )data;
        for ( ClientsByConnection::iterator itr = sr->_clients_by_connection.begin(); itr != sr->_clients_by_connection.end(); ++itr )
        {
                Client *client = itr->second;
                if ( !sr->_qcr.is_connection_ok( client->connection ) )
                {
                        sr->handle_client_disconnect( client );
                        break;
                }
        }
        return AsyncTask::DS_cont;
}

void ServerRepository::
send_to_zone_except( ZONEID_TYPE zone_id, NetDatagram &datagram, ClientVec &exception_list )
{
        if ( _zones_to_clients.find( zone_id ) != _zones_to_clients.end() )
        {
                ClientVec clients = _zones_to_clients[zone_id];
                for ( size_t i = 0; i < clients.size(); i++ )
                {
                        Client *client = clients[i];
                        if ( find( exception_list.begin(), exception_list.end(), client ) == exception_list.end() )
                        {
                                _cw.send( datagram, client->connection );
                        }
                }
        }
}

void ServerRepository::
send_to_all_except( NetDatagram &datagram, ClientVec &exception_list )
{

        ClientVec clients;
        for ( ClientsByConnection::iterator itr = _clients_by_connection.begin(); itr != _clients_by_connection.end(); ++itr )
        {
                clients.push_back( itr->second );
        }
        for ( size_t i = 0; i < clients.size(); i++ )
        {
                Client *client = clients[i];
                if ( find( exception_list.begin(), exception_list.end(), client ) == exception_list.end() )
                {
                        _cw.send( datagram, client->connection );
                }
        }
}