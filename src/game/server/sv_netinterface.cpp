#include "sv_netinterface.h"
#include "netmessages.h"
#include "serverbase.h"
#include "entregistry.h"
#include "clockdelta.h"
#include "baseplayer.h"
#include "usercmd.h"
#include "globalvars_server.h"

#include <configVariableInt.h>
#include <datagramIterator.h>
#include <clockObject.h>
#include <pStatCollector.h>
#include <pStatTimer.h>

static PStatCollector snapshot_collector( "Server:Snapshot" );

NotifyCategoryDef( server, "" )

static ConfigVariableInt server_port( "server_port", 27015 );
static ConfigVariableDouble sv_heartbeat_tolerance( "sv_heartbeat_tolerance", 20.0 );

ServerNetInterface::ServerNetInterface() :
	// Reserve 0 for the world
	_entnum_alloc( 1, ENTID_MAX ),
	_client_id_alloc( 0, 0xFFFF ),
	_listen_socket( k_HSteamListenSocket_Invalid ),
	_poll_group( k_HSteamNetPollGroup_Invalid ),
	_is_started( false )
{
}

void ServerNetInterface::run_networking_events()
{
	SteamNetworkingSockets()->RunCallbacks( this );
}

int ServerNetInterface::remove_client( Client *cl )
{
	if ( cl->get_player() )
	{
		remove_entity( cl->get_player() );
	}
	_client_id_alloc.free( cl->get_client_id() );
	return (int)_clients_by_address.erase( cl->get_connection() );
}

void ServerNetInterface::remove_entity( CBaseEntity *ent )
{
	server_cat.debug() << "removing entity " << ent << std::endl;
	ent->despawn();
	_entnum_alloc.free( ent->get_entnum() );

	Datagram dg = BeginMessage( NETMSG_DELETE_ENTITY );
	dg.add_uint32( ent->get_entnum() );
	broadcast_datagram( dg, true );

	_entlist.erase( _entlist.find( ent->get_entnum() ) );
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

	PT( CBaseEntity ) plyr = make_player();
	if ( plyr->is_of_type( CBasePlayer::get_class_type() ) )
	{
		cl->set_player( (CBasePlayer *)( plyr.p() ) );
	}
	else
	{
		printf( "Invalid player created!\n" );
		remove_client( cl );
		return;
	}

	NetDatagram dg = BeginMessage( NETMSG_HELLO_RESP );
	dg.add_float32( sv_tickrate );
	dg.add_uint16( cl->get_client_id() );
	dg.add_uint32( plyr->get_entnum() );
	dg.add_string( sv->_map );
	send_datagram( dg, cl->get_connection() );

	update_client_state( cl, CLIENTSTATE_NONE );
}

PT( CBaseEntity ) ServerNetInterface::make_player()
{
	return make_entity_by_name( "baseplayer" );
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

void ServerNetInterface::handle_datagram( Datagram &dg, Client *cl )
{
	DatagramIterator dgi( dg );

	int msgtype = dgi.get_uint16();

	switch ( msgtype )
	{
	case NETMSG_CLIENT_STATE:
	{
		int state = dgi.get_uint8();
		update_client_state( cl, state );
		break;
	}
	case NETMSG_CMD:
	{
		std::string cmd = dgi.get_string();
		process_cmd( cl, cmd );
		break;
	}
	case NETMSG_USERCMD:
	{
		cl->handle_cmd( dgi );
		break;
	}
	case NETMSG_TICK:
	{
		int tick = dgi.get_int32();
		float frametime = dgi.get_float32();
		cl->set_tick( tick, frametime );
		break;
	}
	default:
		break;
	}
}

void ServerNetInterface::read_incoming_messages()
{
	ISteamNetworkingMessage *incoming_msg = nullptr;
	int num_msgs = SteamNetworkingSockets()->ReceiveMessagesOnPollGroup( _poll_group, &incoming_msg, 1 );
	if ( num_msgs == 0 )
		return;
	if ( num_msgs < 0 )
	{
		server_cat.error() << "Error checking for messages" << std::endl;
		return;
	}

	nassertv( num_msgs == 1 && incoming_msg );
	auto iclient = _clients_by_address.find( incoming_msg->GetConnection() );
	nassertv( iclient != _clients_by_address.end() );
	Client *cl = iclient->second;

	Datagram dg;
	dg.append_data( incoming_msg->m_pData, incoming_msg->m_cbSize );
	handle_datagram( dg, cl );

	incoming_msg->Release();
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

bool ServerNetInterface::startup()
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

PT( CBaseEntity ) ServerNetInterface::make_entity_by_name( const std::string &name, bool spawn,
					      bool bexplicit_entnum, entid_t explicit_entnum )
{
	CBaseEntity *singleton = CEntRegistry::ptr()->_networkname_to_class[name];
	PT( CBaseEntity ) ent = singleton->make_new();
	if ( bexplicit_entnum )
		ent->init( explicit_entnum );
	else
		ent->init( _entnum_alloc.allocate() );
	ent->precache();

	if ( spawn )
		ent->spawn();

	_entlist.insert( entmap_t::value_type( ent->get_entnum(), ent ) );

	server_cat.debug() << "Made new " << name << ", entnum" << ent->get_entnum() << std::endl;

	return ent;
}

void ServerNetInterface::send_datagram( Datagram &dg, HSteamNetConnection conn )
{
	SteamNetworkingSockets()->SendMessageToConnection( conn, dg.get_data(),
							   dg.get_length(),
							   k_nSteamNetworkingSend_Reliable, nullptr );
}

void ServerNetInterface::build_snapshot( Datagram &dg, bool full_snapshot )
{
	PStatTimer timer( snapshot_collector );

	dg.add_uint16( NETMSG_SNAPSHOT );
	dg.add_uint32( _entlist.size() );

	for ( auto itr = _entlist.begin();
	      itr != _entlist.end(); ++itr )
	{
		CBaseEntity *ent = itr->second;
		ServerClass *cls = ent->get_server_class();
		SendTable &st = cls->get_send_table();

		dg.add_uint32( ent->get_entnum() );
		dg.add_string( cls->get_network_name() );

		int num_props;
		if ( full_snapshot || ent->is_entity_fully_changed() )
			num_props = st.get_num_props();
		else
			num_props = ent->get_num_changed_offsets();

		dg.add_uint16( num_props );

		int total_props = st.get_num_props();
		for ( int j = 0; j < total_props; j++ )
		{
			SendProp &prop = st.get_send_props()[j];
			if ( full_snapshot || ent->is_property_changed( &prop ) )
			{
				
				dg.add_string( prop.get_prop_name() );
				void *data = (unsigned char *)ent + prop.get_offset();
				prop.get_proxy()( &prop, ent, data, dg );
			}
		}

		if ( !full_snapshot )
			ent->reset_changed_offsets();
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
	build_snapshot( dg, true );
	send_datagram( dg, cl->get_connection() );
}

void ServerNetInterface::snapshot()
{
	Datagram tickdg = build_tick();
	broadcast_datagram( tickdg, true );

	Datagram dg;
	build_snapshot( dg, false );

	// Send snapshot to all clients
	broadcast_datagram( dg, true );
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