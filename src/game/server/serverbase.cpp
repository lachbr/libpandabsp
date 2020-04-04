#include "serverbase.h"
#include "netmessages.h"
#include "sv_bsploader.h"
#include "sv_netinterface.h"
#include "sv_entitymanager.h"
#include "physicssystem.h"
#include "baseplayer.h"

#include <configVariableDouble.h>
#include <pStatClient.h>

ServerBase *sv = nullptr;

ServerBase::ServerBase()
	: HostBase(),
	_root( "ServerRoot" )
{
	sv = this;
}

void ServerBase::mount_multifiles()
{
	HostBase::mount_multifiles();

	mount_multifile( "resources/phase_0.mf" );
	mount_multifile( "resources/phase_3.mf" );
	mount_multifile( "resources/phase_3.5.mf" );
	mount_multifile( "resources/phase_4.mf" );
	mount_multifile( "resources/phase_5.mf" );
	mount_multifile( "resources/phase_5.5.mf" );
	mount_multifile( "resources/phase_6.mf" );
	mount_multifile( "resources/phase_7.mf" );
	mount_multifile( "resources/phase_8.mf" );
	mount_multifile( "resources/phase_9.mf" );
	mount_multifile( "resources/phase_10.mf" );
	mount_multifile( "resources/phase_11.mf" );
	mount_multifile( "resources/phase_12.mf" );
	mount_multifile( "resources/phase_13.mf" );
	mount_multifile( "resources/phase_14.mf" );
	mount_multifile( "resources/mod.mf" );
}

void ServerBase::run_cmd( const std::string &full_cmd, Client *cl )
{
}

void ServerBase::add_game_systems()
{
	PT( ServerNetInterface ) net = new ServerNetInterface;
	add_game_system( net );

	PT( ServerEntitySystem ) ents = new ServerEntitySystem;
	net->add_datagram_handler( ents );
	add_game_system( ents );

	PT( ServerBSPSystem ) bsp = new ServerBSPSystem;
	add_game_system( bsp );

	PT( PhysicsSystem ) phys = new PhysicsSystem;
	add_game_system( phys );

	net->initialize();
	ents->initialize();
	phys->initialize();
	bsp->initialize();
}

/**
 * Run a frame from the server.
 */
void ServerBase::end_tick()
{
	// We need to build our snapshot at the very end of the tick.
	svnet->send_tick();
	svnet->snapshot();
}

void ServerBase::load_cfg_files()
{
	HostBase::load_cfg_files();
	load_cfg_file( "cfg/server.cfg" );
}