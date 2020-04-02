#include "clientbase.h"

#include "in_buttons.h"
#include <keyboardButton.h>
#include <mouseButton.h>
#include <dynamicTextFont.h>

// Game systems we need
#include "cl_rendersystem.h"
#include "inputsystem.h"
#include "audiosystem.h"
#include "physicssystem.h"
#include "intervalsystem.h"
#include "cl_entitymanager.h"
#include "cl_netinterface.h"
#include "cl_bsploader.h"

ClientBase *cl = nullptr;

static ConfigVariableBool cl_showfps( "cl_showfps", false );
static ConfigVariableInt fps_max( "fps_max", 300 );

ClientBase::ClientBase() :
	HostBase(),	
	_fps_meter( new CFrameRateMeter )
{
	cl = this;
}

void ClientBase::run_cmd( const std::string &full_cmd )
{
	vector_string args = parse_cmd( full_cmd );
	std::string cmd = args[0];
	args.erase( args.begin() );

	if ( cmd == "map" )
	{
		if ( args.size() > 0 )
		{
			std::string mapname = args[0];
			start_game( mapname );
		}
	}
	else
	{
		// Don't know this command from client,
		// send it to the server.
		//Datagram dg = BeginMessage( NETMSG_CMD );
		//dg.add_string( full_cmd );
		//_client->send_datagram( dg );
	}
}

void ClientBase::start_game( const std::string &mapname )
{
	// Start a local host game

	// Start the server process
	FILE *hserver = _popen( std::string( "p3d_server.exe +map " + mapname ).c_str(), "wt" );
	if ( hserver == NULL )
	{
		nassert_raise( "Could not start server process!" );
		return;
	}

	// Connect the client to the server
	NetAddress naddr;
	naddr.set_host( "127.0.0.1", 27015 );

	ClientNetInterface *net;
	get_game_system( net );
	net->connect( naddr );
}

void ClientBase::add_game_systems()
{
	// Per-tick systems
	PT( ClientNetInterface ) netsys = new ClientNetInterface;
	add_game_system( netsys, -50 );

	PT( InputSystem ) input = new InputSystem( this );
	add_game_system( input, -40 );

	PT( IntervalSystem ) intervals = new IntervalSystem;
	add_game_system( intervals, -30 );

	PT( ClientEntitySystem ) entities = new ClientEntitySystem;
	netsys->add_datagram_handler( entities );
	add_game_system( entities, -20 );

	PT( PhysicsSystem ) physics = new PhysicsSystem;
	add_game_system( physics, -10 );

	PT( ClientBSPSystem ) bsp = new ClientBSPSystem;
	netsys->add_datagram_handler( bsp );
	add_game_system( bsp, 0 );

	// Per-frame systems (after all ticks)
	PT( ClientRenderSystem ) render = new ClientRenderSystem;
	// InputSystem relies on RenderSystem, so make sure we
	// initialize RenderSystem before InputSystem.
	add_game_system( render, -31, true );
	
	PT( AudioSystem ) audio = new AudioSystem;
	add_game_system( audio, 20, true );
}

void ClientBase::do_frame()
{	
	HostBase::do_frame();
}

void ClientBase::mount_multifiles()
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

bool ClientBase::startup()
{
	if ( !HostBase::startup() )
	{
		return false;
	}

	// Set default mappings
	InputSystem *isys;
	get_game_system( isys );
	isys->add_mapping( IN_FORWARD, KeyboardButton::ascii_key( 'w' ) );
	isys->add_mapping( IN_BACK, KeyboardButton::ascii_key( 's' ) );
	isys->add_mapping( IN_MOVELEFT, KeyboardButton::ascii_key( 'a' ) );
	isys->add_mapping( IN_MOVERIGHT, KeyboardButton::ascii_key( 'd' ) );
	isys->add_mapping( IN_WALK, KeyboardButton::alt() );
	isys->add_mapping( IN_SPEED, KeyboardButton::shift() );
	isys->add_mapping( IN_JUMP, KeyboardButton::space() );
	isys->add_mapping( IN_DUCK, KeyboardButton::control() );
	isys->add_mapping( IN_RELOAD, KeyboardButton::ascii_key( 'r' ) );
	isys->add_mapping( IN_ATTACK, MouseButton::one() );
	isys->add_mapping( IN_ATTACK2, MouseButton::three() );

	TextNode::set_default_font( new DynamicTextFont( "models/fonts/cour.ttf" ) );

	if ( cl_showfps )
	{
		_fps_meter->enable();
	}

	return true;
}

void ClientBase::shutdown()
{
	HostBase::shutdown();

	_fps_meter->disable();
	_fps_meter = nullptr;
}

void ClientBase::load_cfg_files()
{
	HostBase::load_cfg_files();
	load_cfg_file( "cfg/client.cfg" );
}