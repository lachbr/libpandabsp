#include "cl_netinterface.h"
#include "netmessages.h"
#include "clientbase.h"
#include "usercmd.h"
#include "cl_bsploader.h"
#include "cl_entitymanager.h"
#include "network_class.h"

#include <datagramIterator.h>
#include <clockObject.h>
#include <configVariableDouble.h>
#include <pStatCollector.h>

NotifyCategoryDef( c_client, "" )

ClientNetInterface *clnet = nullptr;

IMPLEMENT_CLASS( ClientNetInterface )

static ConfigVariableDouble cl_heartbeat_rate( "cl_heartbeat_rate", 1.0 );
static ConfigVariableDouble cl_sync_rate( "cl_sync_rate", 30.0 );
static ConfigVariableDouble cl_max_uncertainty( "cl_max_uncertainty", 0.05 );

ClientNetInterface::ClientNetInterface() :
	_connection( k_HSteamNetConnection_Invalid ),
	_connected( false ),
	_client_state( CLIENTSTATE_NONE ),
	_server_tickrate( 0.0f ),
	_last_server_tick_time( 0.0f ),
	_oldtickcount( 0 ),
	_tick_remainder( 0.0f ),
	_server_tick( 0 ),
	_server_frametime( 0.0f )
{
	clnet = this;
	
}

bool ClientNetInterface::initialize()
{
	SteamNetworkingErrMsg msg;
	GameNetworkingSockets_Init( nullptr, msg );

	return true;
}

void ClientNetInterface::shutdown()
{
}

void ClientNetInterface::OnSteamNetConnectionStatusChanged( SteamNetConnectionStatusChangedCallback_t *info )
{
	nassertv( info->m_hConn == _connection || _connection == k_HSteamNetConnection_Invalid );

	switch ( info->m_info.m_eState )
	{
	case k_ESteamNetworkingConnectionState_ClosedByPeer:
	case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
		c_client_cat.warning() << "lost connection" << std::endl;
		SteamNetworkingSockets()->CloseConnection( info->m_hConn, 0, nullptr, false );
		_connection = k_HSteamNetConnection_Invalid;
		_connected = false;
		break;
	case k_ESteamNetworkingConnectionState_Connected:
		c_client_cat.info() << "Connect success" << std::endl;
		_connected = true;
		break;
	default:
		break;
	}
}

void ClientNetInterface::connect( const NetAddress &addr )
{
	c_client_cat.info() << "Connecting to " << addr << std::endl;

	_server_addr = addr;
	SteamNetworkingIPAddr saddr;
	saddr.Clear();
	saddr.ParseString( addr.get_addr().get_ip_port().c_str() );

	_connection = SteamNetworkingSockets()->ConnectByIPAddress( saddr, 0, nullptr );
	if ( _connection == k_HSteamNetConnection_Invalid )
	{
		c_client_cat.error() << "Failed to connect to " << addr << std::endl;
		return;
	}
	_connected = true;
}

void ClientNetInterface::connect_success( DatagramIterator &dgi )
{
	uint16_t comp_count = dgi.get_uint16();

	for ( size_t i = 0; i < comp_count; i++ )
	{
		std::string network_name = dgi.get_string();
		uint16_t comp_id = dgi.get_uint16();

		BaseComponentShared *comp = clents->get_component_from_networkname( network_name );
		if ( !comp )
		{
			nassert_raise( "Component missing from client network tables!" );
			return;
		}

		NetworkClass *nclass = comp->get_network_class();
		nclass->set_id( comp_id );

		uint8_t prop_count = dgi.get_uint8();
		for ( size_t j = 0; j < prop_count; j++ )
		{
			std::string prop_name = dgi.get_string();
			uint8_t prop_id = dgi.get_uint8();

			auto itr = nclass->find_inherited_prop( prop_name );
			if ( itr == nclass->inherited_props_end() )
			{
				nassert_raise( "Property missing from client component!" );
				return;
			}
			NetworkProp *prop = *itr;
			prop->set_id( prop_id );
		}

		c_client_cat.info()
			<< nclass->get_name() << " has ID " << nclass->get_id() << std::endl;
		clents->register_component( comp );
	}

	c_client_cat.info() << "Connected to " << _server_addr << std::endl;

	cl->set_tick_rate( dgi.get_uint8() );
	_my_client_id = dgi.get_uint16();

	ClientEntitySystem *esys;
	ClientBSPSystem *bsys;
	cl->get_game_system( esys );
	cl->get_game_system( bsys );

	esys->set_local_player_id( dgi.get_uint32() );
	std::string mapname = dgi.get_string();
	bsys->load_bsp_level( bsys->get_map_filename( mapname ) );
}

void ClientNetInterface::set_client_state( int state )
{
	_client_state = state;
	Datagram dg = BeginMessage( NETMSG_CLIENT_STATE );
	dg.add_uint8( _client_state );
	send_datagram( dg );
}

void ClientNetInterface::send_datagram( Datagram &dg )
{
	SteamNetworkingSockets()->SendMessageToConnection( _connection, dg.get_data(), dg.get_length(),
							   k_nSteamNetworkingSend_Reliable, nullptr );
}

void ClientNetInterface::handle_datagram( const Datagram &dg )
{
	DatagramIterator dgi( dg );
	int msgtype = dgi.get_uint16();

	switch ( msgtype )
	{
	case NETMSG_HELLO_RESP:
		connect_success( dgi );
		break;
	case NETMSG_SERVER_HEARTBEAT:
		// todo
		break;
	case NETMSG_TICK:
	{
		handle_server_tick( dgi );
		break;
	}
	default:
	{
		// Don't have our own handler for this, delegate to our datagram handlers
		for ( size_t i = 0; i < _datagram_handlers.size(); i++ )
		{
			if ( _datagram_handlers[i]->handle_datagram( msgtype, dgi ) )
			{
				break;
			}
		}
		break;
	}
	}
}

void ClientNetInterface::handle_server_tick( DatagramIterator &dgi )
{
	int servertick = dgi.get_int32();
	float frametime = dgi.get_float32();
	_server_tick = servertick;
	_server_frametime = frametime;
}

void ClientNetInterface::send_tick()
{
	Datagram dg = BeginMessage( NETMSG_TICK );
	dg.add_int32( _server_tick );
	dg.add_float32( cl->get_frametime() );
	send_datagram( dg );
}

void ClientNetInterface::read_incoming_messages()
{
	ISteamNetworkingMessage *msg;
	int num_msgs;
	Datagram dg;

	// Read until there are no more messages
	while ( true )
	{
		num_msgs = SteamNetworkingSockets()->ReceiveMessagesOnConnection( _connection, &msg, 1 );
		if ( !msg || num_msgs <= 0 )
			break;

		dg = Datagram( msg->m_pData, msg->m_cbSize );
		handle_datagram( dg );

		msg->Release();
	}
}

void ClientNetInterface::update( double frametime )
{
	if ( _connected )
	{
		read_incoming_messages();
	}
	SteamNetworkingSockets()->RunCallbacks( this );
}
