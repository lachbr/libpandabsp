#include "serverbase.h"
#include "netmessages.h"
#include "sv_bsploader.h"
#include "baseplayer.h"

#include <configVariableDouble.h>
#include <pStatClient.h>

ServerBase *sv = nullptr;

static int host_frameticks = 0;
static int host_tickcount  = 0;
static int host_currentframetick = 0;

ServerBase::ServerBase()
	: HostBase()
	, _server( new ServerNetInterface )
{
	sv = this;
}

void ServerBase::run_cmd( const std::string &full_cmd, Client *cl )
{
}

void ServerBase::load_bsp_level( const Filename &path, bool is_transition )
{
	Datagram dg = BeginMessage( NETMSG_CHANGE_LEVEL );
	dg.add_string( path.get_basename_wo_extension() );
	dg.add_uint8( (int)is_transition );
	_server->broadcast_datagram( dg );

	HostBase::load_bsp_level( path, is_transition );
}

void ServerBase::make_bsp()
{
	_bsp_loader = new CSV_BSPLoader;
}

void ServerBase::setup_bsp()
{
	_bsp_loader->set_ai( true );
}

bool ServerBase::startup()
{
	if ( !HostBase::startup() )
		return false;

	if ( !_server->startup() )
		return false;

	return true;
}

void ServerBase::run_entities( bool simulating )
{
	if ( simulating )
	{
		for ( auto itr : *_server->get_entlist() )
		{
			CBaseEntity *ent = itr.second;
			ent->run_thinks();
		}
	}
	else
	{
		// Only simulate players
		ServerNetInterface::clientmap_t *clients = _server->get_clients();
		for ( auto itr : *clients )
		{
			Client *cl = itr.second;
			if ( cl->get_player() )
				cl->get_player()->run_thinks();
		}
	}
}

void ServerBase::do_net_tick()
{
	_server->read_incoming_messages();
	_server->run_networking_events();
}

/**
 * Run a frame from the server.
 */
void ServerBase::do_game_tick()
{

	bool simulating = !is_paused();

	PandaNode::reset_all_prev_transform();
	_event_mgr->process_events();
	CIntervalManager::get_global_ptr()->step();
		
	// Run entities
	run_entities( simulating );

	// Simulate Bullet physics
	_physics_world->do_physics( _frametime );

	send_tick();
	_server->snapshot();
}

void ServerBase::send_tick()
{
	Datagram dg = BeginMessage( NETMSG_TICK );
	dg.add_int32( _tickcount );
	dg.add_float32( _intervalpertick );
	_server->broadcast_datagram( dg );
}

void ServerBase::load_cfg_files()
{
	HostBase::load_cfg_files();
	load_cfg_file( "cfg/server.cfg" );
}

void ServerBase::setup_tasks()
{
}