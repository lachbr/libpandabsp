#include "clientrepositorybase.h"
#include "pp_globals.h"
#include "timemanager.h"
#include "distributedobject_base.h"
#include "distributednode.h"

#include <configVariableDouble.h>
#include <throw_event.h>

#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

NotifyCategoryDef( clientRepositoryBase, "" );

ClientRepositoryBase::ClientRepositoryBase()
{
}

ClientRepositoryBase::ClientRepositoryBase( const string &dc_suffix,
                                            ConnectionRepository::ConnectMethod cm,
                                            vector<string> &dc_files ) :
        ConnectionRepository( dc_files, cm, false )
{

        set_client_datagram( true );
        _no_defer = true;
        _dc_suffix = dc_suffix;
        _context = 100000;
        _time_mgr = nullptr;
        _local_avatar_do_id = 0;
        _last_heartbeat = 0.0;
        _last_generate = 0.0;
        _server_delta = 0.0;
        _heartbeat_started = false;
        _context = 0;
        _heartbeat_task = nullptr;
        _heartbeat_interval = ConfigVariableDouble( "heartbeat-interval", 10 );
}

void ClientRepositoryBase::set_local_avatar_do_id( DOID_TYPE do_id )
{
        _local_avatar_do_id = do_id;
}

DOID_TYPE ClientRepositoryBase::get_local_avatar_do_id() const
{
        return _local_avatar_do_id;
}

ParentMgr &ClientRepositoryBase::get_parent_mgr()
{
        return _parent_mgr;
}

void ClientRepositoryBase::set_time_manager( TimeManager *tm )
{
        _time_mgr = tm;
}

TimeManager *ClientRepositoryBase::get_time_manager() const
{
        return _time_mgr;
}

int ClientRepositoryBase::allocate_context()
{
        return ++_context;
}

void ClientRepositoryBase::set_server_delta( PN_stdfloat delta )
{
        _server_delta = delta;
}

PN_stdfloat ClientRepositoryBase::get_server_delta() const
{
        return _server_delta;
}

PN_stdfloat ClientRepositoryBase::get_server_time_of_day() const
{
#ifdef _WIN32
#pragma message("Using _WIN32 system time function")
        SYSTEMTIME time;
        GetSystemTime( &time );
        LONG time_ms = ( time.wSecond * 1000 ) + time.wMilliseconds;

        return ( time_ms / 1000.0f ) + _server_delta;
#else
#pragma message("WARNING: No implementation for ClientRepositoryBase::get_server_time_of_day() on this platform.")
        return 0.0f;
#endif // _WIN32
}

void ClientRepositoryBase::do_generate( uint32_t parent_id, uint32_t zone_id, uint16_t class_id,
                                        uint32_t do_id, DatagramIterator &di )
{
        DCClassPP *dclass = _dclass_by_number[class_id];
        dclass->get_dclass()->start_generate();
        DistributedObjectBase *dobj = generate_with_required_other_fields( dclass, do_id, di, parent_id, zone_id );
        dclass->get_dclass()->stop_generate();
}

DistributedObjectBase *ClientRepositoryBase::generate_with_required_fields( DCClassPP *dclass, uint32_t do_id,
                                                                            DatagramIterator &di, uint32_t parent_id,
                                                                            uint32_t zone_id )
{
        DistributedObjectBase *dobj;
        if ( _do_id_2_do.find( do_id ) != _do_id_2_do.end() )
        {
                dobj = _do_id_2_do[do_id];
                dobj->generate();
                dobj->set_location( parent_id, zone_id );
                dobj->update_required_fields( dclass, di );
        }
        else
        {
                dobj = dclass->get_obj_singleton()->make_new();
                dobj->set_dclass( dclass );
                dobj->set_do_id( do_id );
                _do_id_2_do[do_id] = dobj;
                dobj->generate_init();
                dobj->generate();
                dobj->set_location( parent_id, zone_id );
                dobj->update_required_fields( dclass, di );
        }

        return dobj;
}

DistributedObjectBase *ClientRepositoryBase::generate_with_required_other_fields( DCClassPP *dclass, uint32_t do_id,
                                                                                  DatagramIterator &di, uint32_t parent_id,
                                                                                  uint32_t zone_id )
{
        DistributedObjectBase *dobj;
        if ( _do_id_2_do.find( do_id ) != _do_id_2_do.end() )
        {
                dobj = _do_id_2_do[do_id];
                dobj->generate();
                dobj->set_location( parent_id, zone_id );
                dobj->update_required_other_fields( dclass, di );
        }
        else
        {
                dobj = dclass->get_obj_singleton()->make_new();
                dobj->set_dclass( dclass );
                dobj->set_do_id( do_id );
                _do_id_2_do[do_id] = dobj;
                dobj->generate_init();
                dobj->generate();
                dobj->set_location( parent_id, zone_id );
                dobj->update_required_other_fields( dclass, di );
        }

        return dobj;
}

void ClientRepositoryBase::disable_do_id( uint32_t do_id, bool owner_view )
{

}

void ClientRepositoryBase::handle_delete( DatagramIterator &di )
{
}

void ClientRepositoryBase::handle_update_field( DatagramIterator &di )
{
        uint32_t do_id = di.get_uint32();
        do_update( do_id, di, false );
}

void ClientRepositoryBase::do_update( uint32_t do_id, DatagramIterator &di, bool ov_updated )
{
        DistributedObjectBase *dobj = _do_id_2_do[do_id];
        dobj->get_dclass()->receive_update( dobj, di );
}

void ClientRepositoryBase::handle_go_get_lost( DatagramIterator &di )
{
        if ( di.get_remaining_size() > 0 )
        {
                _booted_index = di.get_uint16();
                _booted_text = di.get_string();

                connectionRepository_cat.warning()
                        << "Server is booting us out (" << _booted_index << "): " << _booted_text << "\n";
        }
        else
        {
                connectionRepository_cat.warning()
                        << "Server is booting us out with no explanation.\n";
        }

        stop_reader_poll_task();
        lost_connection();
}

void ClientRepositoryBase::handle_server_heartbeat( DatagramIterator &di )
{
        if ( ConfigVariableBool( "server-heartbeat-info", true ) )
        {
                connectionRepository_cat.info()
                        << "Server heartbeat.\n";
        }
}

string ClientRepositoryBase::handle_system_message( DatagramIterator &di )
{
        string message = di.get_string();
        connectionRepository_cat.info()
                << "Message from server: " << message << "\n";
        return message;
}

void ClientRepositoryBase::handle_system_message_acknowledge( DatagramIterator &di )
{
        string message = di.get_string();
        connectionRepository_cat.info()
                << "Message with acknowledge from server: " << message << "\n";
        throw_event( "system message acknowledge", EventParameter( message ) );
}

ClientRepositoryBase::ObjectsDict ClientRepositoryBase::get_objects_of_class( DistributedObjectBase *cls )
{
        ObjectsDict result;
        for ( ObjectsDict::iterator itr = _do_id_2_do.begin(); itr != _do_id_2_do.end(); ++itr )
        {
                uint32_t do_id = itr->first;
                DistributedObjectBase *obj = itr->second;
                int num_parents = obj->get_type().get_num_parent_classes();
                for ( int i = 0; i < num_parents; i++ )
                {
                        if ( obj->get_type().get_parent_class( i ) == cls->get_type() )
                        {
                                result[do_id] = obj;
                        }
                }
        }

        return result;
}

ClientRepositoryBase::ObjectsDict ClientRepositoryBase::get_objects_of_exact_class( DistributedObjectBase *cls )
{
        ObjectsDict result;
        for ( ObjectsDict::iterator itr = _do_id_2_do.begin(); itr != _do_id_2_do.end(); ++itr )
        {
                uint32_t do_id = itr->first;
                DistributedObjectBase *obj = itr->second;
                if ( obj->get_type() == cls->get_type() )
                {
                        result[do_id] = obj;
                }
        }

        return result;
}

void ClientRepositoryBase::consider_heartbeat()
{
        if ( !_heartbeat_started )
        {
                return;
        }

        double elapsed = g_global_clock->get_real_time() - _last_heartbeat;
        if ( elapsed < 0 || elapsed > _heartbeat_interval )
        {
                connectionRepository_cat.debug()
                        << "Sending heartbeat mid-frame.\n";
                start_heartbeat();
                _last_heartbeat = g_global_clock->get_real_time();
        }
}

void ClientRepositoryBase::stop_heartbeat()
{
        if ( _heartbeat_task != nullptr )
        {
                g_task_mgr->remove( _heartbeat_task );
        }
        _heartbeat_started = false;
}

void ClientRepositoryBase::start_heartbeat()
{
        stop_heartbeat();
        _heartbeat_started = true;
        send_heartbeat();
        wait_for_next_heartbeat();
}

AsyncTask::DoneStatus ClientRepositoryBase::send_heartbeat_task( GenericAsyncTask *task, void *data )
{
        ( (ClientRepositoryBase *)data )->send_heartbeat();
        return AsyncTask::DS_again;
}

void ClientRepositoryBase::wait_for_next_heartbeat()
{
        _heartbeat_task = new GenericAsyncTask( "heartBeat", send_heartbeat_task, this );
        //_heartbeat_task->set_task_chain("net");
        _heartbeat_task->set_delay( _heartbeat_interval );
        g_task_mgr->add( _heartbeat_task );
}

NodePath ClientRepositoryBase::get_world( uint32_t do_id )
{
        DistributedObjectBase *obj = _do_id_2_do[do_id];

        if ( obj->get_type().is_derived_from( DistributedNode::get_class_type() ) )
        {
                DistributedNode *nobj = DCAST( DistributedNode, obj );
                NodePath world_np = nobj->get_parent();
                while ( true )
                {
                        NodePath next_np = world_np.get_parent();
                        if ( next_np == g_render )
                        {
                                break;
                        }
                        else if ( world_np.is_empty() )
                        {
                                return NodePath();
                        }
                }

                return world_np;
        }

        return g_render;
}

bool ClientRepositoryBase::is_local_id( uint32_t do_id )
{
        return false;
}