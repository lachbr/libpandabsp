/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file networksystem.cpp
 * @author Brian Lach
 * @date March 29, 2020
 */

#include "networksystem.h"

NetworkSystem *NetworkSystem::s_pGlobalPtr = nullptr;

NetworkSystem::NetworkSystem()
{
	SteamNetworkingErrMsg errMsg;
	m_pInterface = GameNetworkingSockets_CreateInstance( nullptr, errMsg );
	if ( !m_pInterface )
	{
		networksystem_cat.error()
			<< "Unable to initialize SteamNetworkingSockets! (" << std::string( errMsg ) << ")\n";
		return;
	}
}

NetworkSystem::~NetworkSystem()
{
	if ( m_pInterface )
	{
		GameNetworkingSockets_KillInstance( m_pInterface );
		m_pInterface = nullptr;
	}
}

void NetworkCallbacks::OnSteamNetConnectionStatusChanged( SteamNetConnectionStatusChangedCallback_t *pCallback )
{
#ifdef HAVE_PYTHON
	if ( m_pPyCallback )
	{
		PyObject *pArgs = PyTuple_Pack( 3,
						PyInt_FromSize_t( pCallback->m_hConn ),
						PyInt_FromSize_t( pCallback->m_info.m_eState ),
						PyInt_FromSize_t( pCallback->m_eOldState ) );
		PyObject_CallObject( m_pPyCallback, pArgs );
	}
#endif
}

void NetworkCallbacks::on_connection_status_changed( NetworkConnectionHandle hConn,
						     NetworkSystem::NetworkConnectionState currState,
						     NetworkSystem::NetworkConnectionState oldState )
{
}


void NetworkSystem::close_connection( NetworkConnectionHandle hConn )
{
	m_pInterface->CloseConnection( hConn, 0, nullptr, false );
}

void NetworkSystem::run_callbacks( NetworkCallbacks *pCallbacks )
{
	m_pInterface->RunCallbacks( pCallbacks );
}

bool NetworkSystem::accept_connection( NetworkConnectionHandle hConn )
{
	return m_pInterface->AcceptConnection( hConn ) == k_EResultOK;
}

bool NetworkSystem::set_connection_poll_group( NetworkConnectionHandle hConn, NetworkPollGroupHandle hPollGroup )
{
	return m_pInterface->SetConnectionPollGroup( hConn, hPollGroup );
}

bool NetworkSystem::receive_message_on_connection( NetworkConnectionHandle hConn, NetworkMessage &msg )
{
	ISteamNetworkingMessage *pMsg = nullptr;
	int nMsgCount = m_pInterface->ReceiveMessagesOnConnection( hConn, &pMsg, 1 );
	if ( !pMsg || nMsgCount != 1 )
	{
		return false;
	}

	msg.dg = Datagram( pMsg->m_pData, pMsg->m_cbSize );
	msg.dgi.assign( msg.dg );
	msg.hConn = pMsg->GetConnection();

	return true;
}

bool NetworkSystem::get_connection_info( NetworkConnectionHandle hConn, NetworkConnectionInfo *pInfo )
{
	SteamNetConnectionInfo_t sInfo;
	if ( !m_pInterface->GetConnectionInfo( hConn, &sInfo ) )
	{
		return false;
	}

	pInfo->listenSocket = sInfo.m_hListenSocket;
	pInfo->state = ( NetworkSystem::NetworkConnectionState )sInfo.m_eState;
	pInfo->endReason = sInfo.m_eEndReason;
	char pBuf[100];
	sInfo.m_addrRemote.ToString( pBuf, 100, false );
	if ( !pInfo->netAddress.set_host( pBuf, sInfo.m_addrRemote.m_port ) )
	{
		networksystem_cat.error()
			<< "Unable to set host on NetAddress in get_connection_info()\n";
		return false;
	}

	return true;
}

void NetworkSystem::send_datagram( NetworkConnectionHandle hConn, const Datagram &dg,
				   NetworkSystem::NetworkSendFlags flags )
{
	m_pInterface->SendMessageToConnection(
		hConn, dg.get_data(),
		dg.get_length(), flags, nullptr );
}

NetworkConnectionHandle NetworkSystem::connect_by_IP_address( const NetAddress &addr )
{
	SteamNetworkingIPAddr steamAddr;
	steamAddr.Clear();
	steamAddr.ParseString( addr.get_addr().get_ip_port().c_str() );
	NetworkConnectionHandle handle = m_pInterface->ConnectByIPAddress( steamAddr, 0, nullptr );
	
	return handle;
}

bool NetworkSystem::receive_message_on_poll_group( NetworkPollGroupHandle hPollGroup, NetworkMessage &msg )
{
	ISteamNetworkingMessage *pMsg = nullptr;
	int nMsgCount = m_pInterface->ReceiveMessagesOnPollGroup( hPollGroup, &pMsg, 1 );
	if ( !pMsg || nMsgCount != 1 )
	{
		return false;
	}

	msg.dg = Datagram( pMsg->m_pData, pMsg->m_cbSize );
	msg.dgi.assign( msg.dg );
	msg.hConn = pMsg->GetConnection();

	return true;
}

NetworkPollGroupHandle NetworkSystem::create_poll_group()
{
	return m_pInterface->CreatePollGroup();
}

NetworkListenSocketHandle NetworkSystem::create_listen_socket( int port )
{
	SteamNetworkingIPAddr steamAddr;
	steamAddr.Clear();
	steamAddr.m_port = port;

	return m_pInterface->CreateListenSocketIP( steamAddr, 0, nullptr );
}
