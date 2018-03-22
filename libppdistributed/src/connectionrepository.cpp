#include "connectionrepository.h"
#include "pp_globals.h"
#include "distributedobject_base.h"

#include <httpChannel.h>
#include <dcFile.h>
#include <dcClass.h>
#include <virtualFileSystem.h>

NotifyCategoryDef( connectionRepository, "" );

ConnectionRepository::ConnectionRepository( vector<string> &dc_files, ConnectMethod cm, bool owner_view )
{
        ConfigVariableBool threaded_net = ConfigVariableBool( "threaded-net", false );

        _dc_files = dc_files;

        _connect_method = cm;

        if ( _connect_method == CM_HTTP )
        {
                connectionRepository_cat.info()
                        << "Using connect method 'http'\n";
        }
        else if ( _connect_method == CM_NET )
        {
                connectionRepository_cat.info()
                        << "Using connect method 'net'\n";
        }
        else if ( _connect_method == CM_NATIVE )
        {
                connectionRepository_cat.info()
                        << "Using connect method 'native'\n";
        }

        dc_has_owner_view = owner_view;
        CConnectionRepository::CConnectionRepository( owner_view, threaded_net );
        _dc_suffix = "";
        _server_address = URLSpec( "" );

        // Don't do this python shit:
        set_handle_datagrams_internally( false );
}

ConnectionRepository::ConnectionRepository()
{
}

DistributedObjectBase *ConnectionRepository::generate_global_object( uint32_t do_id, const string &dcname )
{
        DCClassPP *dclass = _dclass_by_name[dcname + _dc_suffix];
        DistributedObjectBase *dobj = dclass->get_obj_singleton()->make_new();
        dobj->set_dclass( dclass );
        dobj->set_do_id( do_id );
        _do_id_2_do[do_id] = dobj;
        dobj->generate_init();
        dobj->generate();
        dobj->announce_generate();
        dobj->set_parent_id( 0 );
        dobj->set_zone_id( 0 );
        return dobj;
}

DCFieldPP *ConnectionRepository::get_field_wrapper( DCField *field )
{
        for ( size_t i = 0; i < _wrapper_fields.size(); i++ )
        {
                DCFieldPP *wrapper_field = _wrapper_fields[i];
                if ( wrapper_field->get_field()->get_number() == field->get_number() )
                {
                        return wrapper_field;
                }
        }

        return (DCFieldPP *)nullptr;
}

void ConnectionRepository::read_dc_file()
{

        dc_file.clear();

        for ( size_t i = 0; i < _dc_files.size(); i++ )
        {
                connectionRepository_cat.info()
                        << "Reading DC file: " << _dc_files[i] << "...\n";
                bool result = dc_file.read( Filename( _dc_files[i] ) );
                if ( !result )
                {
                        connectionRepository_cat.error()
                                << "Could not read dc file!\n";
                        return;
                }
        }

        _hash_val = dc_file.get_hash();

        for ( int i = 0; i < dc_file.get_num_classes(); i++ )
        {
                DCClass *dclass = dc_file.get_class( i );
                int num = dclass->get_number();
                string classname = dclass->get_name() + _dc_suffix;

                DistributedObjectBase *singleton = dc_singleton_by_name[classname];
                if ( singleton == (DistributedObjectBase *)nullptr )
                {
                        connectionRepository_cat.error()
                                << "DClass " << classname << " is not registered as a class in c++ code\n";
                        continue;
                }
                DCClassPP *cls_wrapper = new DCClassPP( dclass );
                cls_wrapper->set_obj_singleton( singleton );

                if ( dc_field_data.find( classname ) != dc_field_data.end() )
                {
                        for ( int j = 0; j < dclass->get_num_inherited_fields(); j++ )
                        {
                                DCField *field = dclass->get_inherited_field( j );
                                string fieldname = field->get_name();
                                DCFieldPP *field_wrapper;
                                if ( find( _wrapped_fields.begin(), _wrapped_fields.end(), field ) == _wrapped_fields.end() )
                                {
                                        field_wrapper = new DCFieldPP( field->as_atomic_field() );
                                        bool has_defined_fields = dc_field_data[classname].find( fieldname ) != dc_field_data[classname].end();
                                        if ( !has_defined_fields )
                                        {
                                                continue;
                                        }
                                        DCFieldFunc *setter = dc_field_data[classname][fieldname][0];
                                        DCFieldFunc *getter = dc_field_data[classname][fieldname][1];
                                        field_wrapper->set_setter_func( setter );
                                        field_wrapper->set_getter_func( getter );
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
                else
                {
                        connectionRepository_cat.error()
                                << "dclass from dcfile " << classname << " not defined in code!\n";
                }

                _dclass_by_name[classname] = cls_wrapper;
                _dclass_by_number[num] = cls_wrapper;
        }

}

DCClassPP *ConnectionRepository::get_dclass_by_name( const string &name ) const
{
        return _dclass_by_name.at( name );
}

void ConnectionRepository::start_reader_poll_task()
{
        stop_reader_poll_task();

        /* accept overflow event */

        _reader_poll_task = new GenericAsyncTask(
                "readerPollTask", reader_poll_task, this );
        _reader_poll_task->set_priority( -30 );

        g_task_mgr->add( _reader_poll_task );

}

void ConnectionRepository::stop_reader_poll_task()
{
        if ( _reader_poll_task != nullptr )
        {
                _reader_poll_task->remove();
                _reader_poll_task = nullptr;
        }
        /* ignore overflow event */
}

void ConnectionRepository::reader_poll_until_empty()
{
        while ( reader_poll_once() )
        {
        }
}

bool ConnectionRepository::reader_poll_once()
{
        if ( check_datagram() )
        {
                get_datagram_iterator( _private_di );
                handle_datagram( _private_di );
                return true;
        }

        if ( !is_connected() )
        {
                stop_reader_poll_task();
                /* send lost connection message */
        }
        return false;
}

AsyncTask::DoneStatus ConnectionRepository::reader_poll_task( GenericAsyncTask *task, void *data )
{
        ( (ConnectionRepository *)data )->reader_poll_until_empty();
        return AsyncTask::DS_cont;
}

void ConnectionRepository::lost_connection()
{
        connectionRepository_cat.warning()
                << "Lost connection to gameserver.\n";
}

void ConnectionRepository::send( Datagram &dg )
{
        if ( dg.get_length() > 0 )
        {
                send_datagram( dg );
        }
}

void ConnectionRepository::request_delete( DistributedObjectBase *obj )
{
        // Nothing happens in this implementation.
}

void ConnectionRepository::handle_update_field()
{
        DOID_TYPE do_id = _private_di.get_uint32();
        DistributedObjectBase *dobj = get_do( do_id );
        nassertv( dobj != nullptr );
        DCClassPP *dclass = dobj->get_dclass();
        nassertv( dclass != nullptr );
        if ( get_in_quiet_zone() )
        {
                if ( !dobj->get_never_disable() )
                {
                        return;
                }
        }
        dclass->receive_update( dobj, _private_di );
}

void ConnectionRepository::handle_update_field_owner()
{
        DOID_TYPE do_id = _private_di.get_uint32();
        DistributedObjectBase *ownerobj = get_owner_view( do_id );
        nassertv( ownerobj != nullptr );
        DCClassPP *ovdclass = ownerobj->get_dclass();
        nassertv( ovdclass != nullptr );
        DCPacker ovpacker;
        ovpacker.set_unpack_data( _private_di.get_remaining_bytes() );
        int ovfield_id = ovpacker.raw_unpack_uint16();
        DCField *ovfield = ovdclass->get_dclass()->get_field_by_index( ovfield_id );
        if ( ovfield->is_ownrecv() )
        {
                DatagramIterator _odi( _private_di );
                ovdclass->receive_update( ownerobj, _odi );
        }

        DistributedObjectBase *dobj = get_do( do_id );
        nassertv( dobj != nullptr );
        DCClassPP *dclass = dobj->get_dclass();
        nassertv( dclass != nullptr );
        DCPacker packer;
        packer.set_unpack_data( _private_di.get_remaining_bytes() );
        int field_id = packer.raw_unpack_uint16();
        DCField *field = dclass->get_dclass()->get_field_by_index( field_id );
        dclass->receive_update( dobj, _private_di );
}

void ConnectionRepository::handle_datagram( DatagramIterator &di )
{
        unsigned int msgtype = get_msg_type();
        switch ( msgtype )
        {
        case CLIENT_OBJECT_SET_FIELD:
        case STATESERVER_OBJECT_SET_FIELD:
                if ( has_owner_view() )
                {
                        handle_update_field_owner();
                }
                else
                {
                        handle_update_field();
                }
                break;
        default:
                break;
        }
}

PT( HTTPClient ) ConnectionRepository::check_http()
{
        if ( _http == nullptr )
        {
                _http = new HTTPClient();
        }

        return _http;
}

void ConnectionRepository::connect( vector<URLSpec> &servers, ConnectCallbackFunc *callback,
                                    ConnectFailCallbackFunc *failcallback, void *data )
{
        bool has_proxy = false;
        string proxies;
        if ( check_http() )
        {
                proxies = _http->get_proxies_for_url( servers[0] );
                has_proxy = ( proxies.compare( "DIRECT" ) != 0 );
        }

        if ( has_proxy )
        {
                connectionRepository_cat.info()
                        << "Connecting to gameserver via proxy list: " << proxies << "\n";
        }
        else
        {
                connectionRepository_cat.info()
                        << "Connecting to gameserver directly (no proxy).\n";
        }

        if ( _connect_method == CM_HTTP )
        {
                PT( HTTPChannel ) ch = _http->make_channel( 0 );
                HTTPConnectProcess *hcp = new HTTPConnectProcess( this, ch, servers, callback, failcallback, data );
        }
        else if ( _connect_method == CM_NET )
        {
                for ( size_t i = 0; i < servers.size(); i++ )
                {
                        URLSpec url = servers[i];
                        connectionRepository_cat.info()
                                << "Connecting to " << url.get_server_and_port() << " via NET interface.\n";
                        if ( try_connect_net( url ) )
                        {
                                start_reader_poll_task();
                                ( *callback )( data );
                                return;
                        }
                }
                ( *failcallback )( data, 0, "", "", 0 );
        }
        else if ( _connect_method == CM_NATIVE )
        {
                connectionRepository_cat.error()
                        << "no support for native net.\n";
                //for (size_t i = 0; i < servers.size(); i++) {
                //  URLSpec url = servers[i];
                //  connectionRepository_cat.info()
                //    << "Connecting to " << url.get_server_and_port() << " via Native interface.\n";
                //  if (connect_native(url)) {
                //    start_reader_poll_task();
                //    (*callback)(data);
                //    return;
                //  }
                //}
                ( *failcallback )( data, 0, "", "", 0 );
        }
        else
        {
                connectionRepository_cat.warning()
                        << "Cannot connect using an unknown connect method.\n";
                ( *failcallback )( data, 0, "", "", 0 );
        }
}

void ConnectionRepository::handle_connected_http( HTTPChannel *ch, URLSpec &url )
{
        set_connection_http( ch );
        _server_address = url;

        connectionRepository_cat.info()
                << "Successfully connected to " << url.get_server_and_port() << ".\n";

        start_reader_poll_task();
}

void ConnectionRepository::disconnect()
{
        connectionRepository_cat.info()
                << "Closing connection to server.\n";
        _server_address = URLSpec( "" );
        CConnectionRepository::disconnect();
        stop_reader_poll_task();
}

void ConnectionRepository::shutdown()
{
        CConnectionRepository::shutdown();
}

void ConnectionRepository::send_add_interest( int handle, int context, DOID_TYPE parentid,
                                              vector<ZONEID_TYPE> zones, const string &desc )
{
        if ( parentid == 0 )
        {
                return;
        }

        Datagram dg;
        if ( zones.size() > (size_t)1 )
        {
                dg.add_uint16( CLIENT_ADD_INTEREST_MULTIPLE );
        }
        else
        {
                dg.add_uint16( CLIENT_ADD_INTEREST );
        }

        dg.add_uint32( context );
        dg.add_uint16( handle );
        dg.add_uint32( parentid );
        if ( zones.size() > (size_t)1 )
        {
                dg.add_uint16( (uint16_t)zones.size() );
        }
        for ( size_t i = 0; i < zones.size(); i++ )
        {
                dg.add_uint32( zones[i] );
        }
        send( dg );
}

void ConnectionRepository::send_remove_interest( int handle, int context )
{
        Datagram dg;
        dg.add_uint16( CLIENT_REMOVE_INTEREST );
        dg.add_uint32( context );
        dg.add_uint16( handle );

        send( dg );
}

void ConnectionRepository::send_remove_ai_interest( int handle )
{
        Datagram dg;
        dg.add_uint16( CLIENT_REMOVE_INTEREST );
        dg.add_uint16( ( 1 << 15 ) + handle );
        send( dg );
}

ConnectionRepository::HTTPConnectProcess::HTTPConnectProcess( ConnectionRepository *repo,
                                                              PT( HTTPChannel ) ch,
                                                              ConnectionRepository::vector_url &servers,
                                                              ConnectionRepository::ConnectCallbackFunc *callback,
                                                              ConnectionRepository::ConnectFailCallbackFunc *failcallback,
                                                              void *data,
                                                              int start_index )
{
        _data = data;
        _repo = repo;
        _ch = ch;
        _index = start_index;
        _callback = callback;
        _fail_callback = failcallback;
        _servers = servers;
        connect_callback();
}

ConnectionRepository::HTTPConnectProcess::~HTTPConnectProcess()
{
}

void ConnectionRepository::HTTPConnectProcess::connect_callback()
{
        URLSpec addr;
        if ( _ch->is_connection_ready() )
        {
                addr = _servers[_index - 1];
                _repo->handle_connected_http( _ch, addr );
                ( *_callback )( _data );
                delete this;
        }
        else if ( (size_t)_index < _servers.size() )
        {
                addr = _servers[_index];
                connectionRepository_cat.info()
                        << "Connecting to " << addr.get_server_and_port() << " via HTTP interface.\n";
                _ch->preserve_status();
                _ch->begin_connect_to( DocumentSpec( addr ) );
                _poll_task = new GenericAsyncTask( "pollConnection", poll_connection_task, this );
                g_task_mgr->add( _poll_task );
        }
        else
        {
                connectionRepository_cat.error()
                        << "Ran out of servers to connect to!\n";
                addr = _servers[_index - 1];
                ( *_fail_callback )( _data, _ch->get_status_code(), _ch->get_status_string(), addr.get_server(), addr.get_port() );
                delete this;
        }
}

AsyncTask::DoneStatus ConnectionRepository::HTTPConnectProcess::poll_connection()
{
        if ( _ch == nullptr )
        {
                connectionRepository_cat.error()
                        << "Shit, _ch is nullptr\n";
        }

        if ( _ch->run() )
        {
                return AsyncTask::DS_cont;
        }
        _index++;
        connect_callback();
        return AsyncTask::DS_done;
}

AsyncTask::DoneStatus ConnectionRepository::HTTPConnectProcess::poll_connection_task( GenericAsyncTask *task, void *data )
{
        HTTPConnectProcess *cls = (HTTPConnectProcess *)data;
        AsyncTask::DoneStatus status = cls->poll_connection();
        return status;
}