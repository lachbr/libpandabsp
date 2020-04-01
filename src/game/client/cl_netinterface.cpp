#include "cl_netinterface.h"
#include "netmessages.h"
#include "clientbase.h"
#include "c_entregistry.h"
#include "clockdelta.h"
#include "usercmd.h"

#include <datagramIterator.h>
#include <clockObject.h>
#include <configVariableDouble.h>
#include <pStatCollector.h>

NotifyCategoryDef( c_client, "" )

ClientNetInterface *g_client = nullptr;

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
	_server_frametime( 0.0f ),
	_cmd_mgr( this )
{
	g_client = this;

	SteamNetworkingErrMsg msg;
	GameNetworkingSockets_Init( nullptr, msg );
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
	c_client_cat.info() << "Connected to " << _server_addr << std::endl;
	cl->set_tick_rate( dgi.get_uint8() );
	_my_client_id = dgi.get_uint16();
	cl->set_local_player_entnum( dgi.get_uint32() );
	std::string mapname = dgi.get_string();
	cl->load_bsp_level( cl->get_map_filename( mapname ) );
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
	case NETMSG_SNAPSHOT:
		receive_snapshot( dgi );
		break;
	case NETMSG_SERVER_HEARTBEAT:
		// todo
		break;
	case NETMSG_DELETE_ENTITY:
	{
		entid_t entnum = dgi.get_uint32();
		cl->get_entity_mgr()->remove_entity( entnum );
		break;
	}
	case NETMSG_CHANGE_LEVEL:
	{
		std::string mapname = dgi.get_string();
		bool is_transition = (bool)dgi.get_uint8();
		cl->load_bsp_level( cl->get_map_filename( mapname ), is_transition );
		break;
	}
	case NETMSG_TICK:
	{
		handle_server_tick( dgi );
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
	ISteamNetworkingMessage *msg = nullptr;
	int num_msgs = SteamNetworkingSockets()->ReceiveMessagesOnConnection( _connection, &msg, 1 );
	if ( num_msgs <= 0 )
		return;

	Datagram dg( msg->m_pData, msg->m_cbSize );
	handle_datagram( dg );

	msg->Release();
}

void ClientNetInterface::tick()
{
	if ( _connected )
	{
		read_incoming_messages();
	}
	SteamNetworkingSockets()->RunCallbacks( this );
}

void ClientNetInterface::receive_snapshot( DatagramIterator &dgi )
{
	ClientEntityManager *mgr = cl->get_entity_mgr();

	pvector<C_BaseEntity *> new_ents;
	pvector<C_BaseEntity *> changed_ents;
	int num_ents = dgi.get_uint32();
	for ( int ient = 0; ient < num_ents; ient++ )
	{
		entid_t entnum = dgi.get_uint32();
		std::string network_name = dgi.get_string();
		int num_props = dgi.get_uint16();

		// Make sure this entity exists, if not, create it.
		bool new_ent = false;
		PT( C_BaseEntity ) ent = mgr->get_entity( entnum );
		if ( !ent )
		{
			ent = mgr->make_entity( network_name, entnum );
			new_ent = true;
		}

		nassertv( ent != nullptr );

		for ( int iprop = 0; iprop < num_props; iprop++ )
		{
			std::string prop_name = dgi.get_string();
			RecvProp *prop = ent->get_client_class()->
				get_recv_table().find_recv_prop( prop_name );
			nassertv( prop != nullptr );
			void *out = (unsigned char *)ent.p() + prop->get_offset();
			prop->get_proxy()( prop, ent, out, dgi );
		}

		if ( new_ent )
			new_ents.push_back( ent );
		if ( num_props > 0 )
			changed_ents.push_back( ent );
	}

	for ( C_BaseEntity *ent : new_ents )
		ent->spawn();
	for ( C_BaseEntity *ent : changed_ents )
		ent->post_data_update();
}

void ClientNetInterface::cmd_tick()
{
	_cmd_mgr.tick();
}