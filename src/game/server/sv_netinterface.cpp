#include "sv_netinterface.h"
#include "netmessages.h"
#include "serverbase.h"
#include "baseplayer.h"
#include "usercmd.h"
#include "sv_entitymanager.h"
#include "sv_bsploader.h"

#include <configVariableInt.h>
#include <datagramIterator.h>
#include <clockObject.h>
#include <pStatCollector.h>
#include <pStatTimer.h>

ServerNetInterface *svnet = nullptr;

IMPLEMENT_CLASS( ServerNetInterface )

static PStatCollector snapshot_collector( "Server:Snapshot" );

NotifyCategoryDef( server, "" )

static ConfigVariableInt server_port( "server_port", 27015 );
static ConfigVariableDouble sv_heartbeat_tolerance( "sv_heartbeat_tolerance", 20.0 );

ServerNetInterface::ServerNetInterface() :
	_client_id_alloc( 1, 0xFFFF ),
	_listen_socket( k_HSteamListenSocket_Invalid ),
	_poll_group( k_HSteamNetPollGroup_Invalid ),
	_is_started( false )
{
	svnet = this;
}

bool ServerNetInterface::initialize()
{
	SteamNetworkingErrMsg err;
	if ( !GameNetworkingSockets_Init( nullptr, err ) )
	{
		server_cat.error() << "ERROR initializing networking library: " << err << std::endl;
		return false;
	}

	SteamNetworkingIPAddr server_local_addr;
	server_local_addr.Clear();
	server_local_addr.m_port = server_port;

	_listen_socket = SteamNetworkingSockets()->CreateListenSocketIP( server_local_addr, 0, nullptr );
	if ( _listen_socket == k_HSteamListenSocket_Invalid )
	{
		server_cat.error() << "Failed to listen on port " << server_port << std::endl;
		return false;
	}

	_poll_group = SteamNetworkingSockets()->CreatePollGroup();
	if ( _poll_group == k_HSteamNetPollGroup_Invalid )
	{
		server_cat.error() << "Failed to listen on port " << server_port << std::endl;
		return false;
	}

	_is_started = true;

	return true;
}

void ServerNetInterface::shutdown()
{
}

void ServerNetInterface::update( double frametime )
{
	read_incoming_messages();
	run_networking_events();

	send_tick();
}

void ServerNetInterface::run_networking_events()
{
	SteamNetworkingSockets()->RunCallbacks( this );
}

int ServerNetInterface::remove_client( Client *cl )
{
	if ( cl->get_player() )
	{
		svents->remove_entity( cl->get_player()->get_entnum() );
	}
	_client_id_alloc.free( cl->get_client_id() );
	_clients_by_id.remove( cl->get_client_id() );
	return (int)_clients_by_address.erase( cl->get_connection() );
}

void ServerNetInterface::handle_client_hello( SteamNetConnectionStatusChangedCallback_t *info )
{
	nassertv( _clients_by_address.find( info->m_hConn ) == _clients_by_address.end() );

	printf( "Got connection from %s\n", info->m_info.m_szConnectionDescription );

	// Try to accept the connection
	if ( SteamNetworkingSockets()->AcceptConnection( info->m_hConn ) != k_EResultOK )
	{
		SteamNetworkingSockets()->CloseConnection( info->m_hConn, 0, nullptr, false );
		printf( "Couldn't accept connection.\n" );
		return;
	}

	// Assign the poll group
	if ( !SteamNetworkingSockets()->SetConnectionPollGroup( info->m_hConn, _poll_group ) )
	{
		SteamNetworkingSockets()->CloseConnection( info->m_hConn, 0, nullptr, false );
		printf( "Failed to set poll group on connection.\n" );
		return;
	}

	PT( Client ) cl = new Client( info->m_info.m_addrRemote, info->m_hConn, _client_id_alloc.allocate() );
	_clients_by_address[info->m_hConn] = cl;
	_clients_by_id[cl->get_client_id()] = cl;

	PT( CBaseEntity ) plyr = make_player();
	if ( plyr->is_of_type( CBasePlayer::get_class_type() ) )
	{
		plyr->set_owner_client( cl->get_client_id() );
		cl->set_player( (CBasePlayer *)( plyr.p() ) );
	}
	else
	{
		printf( "Invalid player created!\n" );
		remove_client( cl );
		return;
	}

	NetDatagram dg = BeginMessage( NETMSG_HELLO_RESP );
	dg.add_uint8( sv_tickrate );
	dg.add_uint16( cl->get_client_id() );
	dg.add_uint32( plyr->get_entnum() );
	dg.add_string( svbsp->get_map() );
	send_datagram( dg, cl->get_connection() );

	update_client_state( cl, CLIENTSTATE_NONE );
}

void ServerNetInterface::send_tick()
{
	Datagram dg = BeginMessage( NETMSG_TICK );
	dg.add_int32( sv->get_tickcount() );
	dg.add_float32( sv->get_interval_per_tick() );
	broadcast_datagram( dg );
}

PT( CBaseEntity ) ServerNetInterface::make_player()
{
	return svents->make_entity_by_name( "baseplayer" );
}

void ServerNetInterface::update_client_state( Client *cl, int state )
{
	cl->set_client_state( state );

	if ( state == CLIENTSTATE_PLAYING )
	{
		// Client has finished loading and started playing.
		// Send all entities.
		server_cat.debug() << "Send full snapshot" << std::endl;
		send_full_snapshot_to_client( cl );
	}
}

void ServerNetInterface::process_cmd( Client *cl, const std::string &cmd )
{
	sv->run_cmd( cmd, cl );
}

void ServerNetInterface::handle_datagram( Datagram &dg, Client *client )
{
	DatagramIterator dgi( dg );

	int msgtype = dgi.get_uint16();

	switch ( msgtype )
	{
	case NETMSG_CLIENT_STATE:
	{
		int state = dgi.get_uint8();
		update_client_state( client, state );
		break;
	}
	case NETMSG_CMD:
	{
		std::string cmd = dgi.get_string();
		process_cmd( client, cmd );
		break;
	}
	case NETMSG_USERCMD:
	{
		client->handle_cmd( dgi );
		break;
	}
	case NETMSG_TICK:
	{
		int tick = dgi.get_int32();
		float frametime = dgi.get_float32();
		client->set_tick( tick, frametime );
		break;
	}
	default:
	{
		// Delegate to one of our datagram handlers
		for ( size_t i = 0; i < _datagram_handlers.size(); i++ )
		{
			if ( _datagram_handlers[i]->handle_datagram( client, msgtype, dgi ) )
			{
				break;
			}
		}
		break;
	}
	}
}

void ServerNetInterface::read_incoming_messages()
{
	ISteamNetworkingMessage *incoming_msg;
	int num_msgs;
	Client *cl;
	Datagram dg;

	while ( true )
	{
		num_msgs = SteamNetworkingSockets()->ReceiveMessagesOnPollGroup( _poll_group, &incoming_msg, 1 );
		if ( !incoming_msg || num_msgs == 0 )
			break;
		if ( num_msgs < 0 )
		{
			server_cat.error() << "Error checking for messages" << std::endl;
			break;
		}

		nassertv( num_msgs == 1 && incoming_msg );
		auto iclient = _clients_by_address.find( incoming_msg->GetConnection() );
		nassertv( iclient != _clients_by_address.end() );
		cl = iclient->second;

		dg = Datagram( incoming_msg->m_pData, incoming_msg->m_cbSize );
		handle_datagram( dg, cl );

		incoming_msg->Release();
	}
	
}

void ServerNetInterface::OnSteamNetConnectionStatusChanged( SteamNetConnectionStatusChangedCallback_t *info )
{
	switch ( info->m_info.m_eState )
	{
	case k_ESteamNetworkingConnectionState_ClosedByPeer:
	case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
		if ( info->m_eOldState == k_ESteamNetworkingConnectionState_Connected )
		{
			auto iclient = _clients_by_address.find( info->m_hConn );
			nassertv( iclient != _clients_by_address.end() );

			// Select appropriate log messages
			const char *pszDebugLogAction;
			if ( info->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally )
			{
				pszDebugLogAction = "lost (Problem detected locally)";
			}
			else
			{
				// Note that here we could check the reason code to see if
				// it was a "usual" connection or an "unusual" one.
				pszDebugLogAction = "lost (Closed by peer)";
			}

			// Spew something to our own log.  Note that because we put their nick
			// as the connection description, it will show up, along with their
			// transport-specific data (e.g. their IP address)
			printf( "Connection %s %s, reason %d: %s\n",
				info->m_info.m_szConnectionDescription,
				pszDebugLogAction,
				info->m_info.m_eEndReason,
				info->m_info.m_szEndDebug
			);

			remove_client( iclient->second );
		}
		break;
	case k_ESteamNetworkingConnectionState_Connecting:
		handle_client_hello( info );
		break;
	default:
		break;
	}
}

void ServerNetInterface::send_datagram( Datagram &dg, HSteamNetConnection conn )
{
	SteamNetworkingSockets()->SendMessageToConnection( conn, dg.get_data(),
							   dg.get_length(),
							   k_nSteamNetworkingSend_Reliable, nullptr );
}

void ServerNetInterface::send_datagram_to_clients( Datagram &dg, const vector_uint32 &client_ids )
{
	size_t count = client_ids.size();
	for ( size_t i = 0; i < count; i++ )
	{
		send_datagram_to_client( dg, client_ids[i] );
	}
}

void ServerNetInterface::send_datagram_to_client( Datagram &dg, uint32_t client_id )
{
	int iidclient = _clients_by_id.find( client_id );
	if ( iidclient != -1 )
	{
		send_datagram( dg, _clients_by_id.get_data( iidclient )->get_connection() );
	}
}

void ServerNetInterface::broadcast_datagram( Datagram &dg, bool only_playing )
{
	for ( auto itr = _clients_by_address.begin();
	      itr != _clients_by_address.end(); ++itr )
	{
		Client *cl = itr->second;

		if ( ( only_playing && cl->get_client_state() == CLIENTSTATE_PLAYING ) ||
		     !only_playing )
			send_datagram( dg, cl->get_connection() );
	}
}

void ServerNetInterface::send_full_snapshot_to_client( Client *cl )
{
	send_tick( cl );
	Datagram dg;
	svents->build_snapshot( dg, cl->get_client_id(), true );
	send_datagram( dg, cl->get_connection() );
}

void ServerNetInterface::snapshot()
{
	Datagram tickdg = build_tick();
	broadcast_datagram( tickdg, true );

	// Build and send a snapshot of the world to each client.
	//
	// We build a separate snapshot for each client because each
	// client may receive a different snapshot depending on if
	// the client owns any entities and the SendProps have the
	// ownrecv flag.
	size_t client_count = _clients_by_id.size();
	for ( size_t i = 0; i < client_count; i++ )
	{
		Client *cl = _clients_by_id.get_data( i );
		if ( cl->get_client_state() != CLIENTSTATE_PLAYING )
			continue;

		Datagram dg;
		if ( svents->build_snapshot( dg, cl->get_client_id(), false ) )
		{
			send_datagram( dg, cl->get_connection() );
		}
	}

	svents->reset_all_changed_props();
}

Datagram ServerNetInterface::build_tick()
{
	Datagram dg = BeginMessage( NETMSG_TICK );
	dg.add_int32( sv->get_tickcount() );
	dg.add_float32( sv->get_frametime() );
	return dg;
}

void ServerNetInterface::send_tick( Client *cl )
{
	Datagram dg = build_tick();
	send_datagram( dg, cl->get_connection() );
}