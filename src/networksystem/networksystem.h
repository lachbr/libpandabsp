/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file networksystem.h
 * @author Brian Lach
 * @date March 29, 2020
 *
 * @desc Thin wrapper layer on top of Valve's GameNetworkingSockets library to
 *       expose it to Python and integrate with some of Panda's interfaces.
 */

#ifndef NETWORKSYSTEM_H
#define NETWORKSYSTEM_H

#include "config_networksystem.h"
#include "referenceCount.h"
#include "pointerTo.h"
#include "datagram.h"
#include "netAddress.h"
#include "datagramIterator.h"
#include "pdeque.h"

#ifdef HAVE_PYTHON
#include "py_panda.h"
#endif

#ifndef CPPPARSER
#include <steam/steamnetworkingsockets.h>
#include <steam/steamnetworkingtypes.h>
#else
class ISteamNetworkingSocketsCallbacks;
class ISteamNetworkingSockets;
#endif

typedef uint32_t NetworkListenSocketHandle;
typedef uint32_t NetworkPollGroupHandle;
typedef uint32_t NetworkConnectionHandle;
static constexpr NetworkConnectionHandle INVALID_NETWORK_CONNECTION_HANDLE = 0U;
static constexpr NetworkListenSocketHandle INVALID_NETWORK_LISTEN_SOCKET_HANDLE = 0U;
static constexpr NetworkPollGroupHandle INVALID_NETWORK_POLL_GROUP_HANDLe = 0U;

class EXPCL_NETWORKSYSTEM NetworkMessage
{
PUBLISHED:
	const Datagram &get_datagram();
	DatagramIterator &get_datagram_iterator();
	NetworkConnectionHandle get_connection();

	Datagram dg;
	DatagramIterator dgi;
	NetworkConnectionHandle hConn;
};

INLINE const Datagram &NetworkMessage::get_datagram()
{
	return dg;
}

INLINE DatagramIterator &NetworkMessage::get_datagram_iterator()
{
	return dgi;
}

INLINE NetworkConnectionHandle NetworkMessage::get_connection()
{
	return hConn;
}

class NetworkConnectionInfo;
class NetworkCallbacks;

class EXPCL_NETWORKSYSTEM NetworkSystem
{
PUBLISHED:
	// Copy of k_nSteamNetworkingSend
	enum NetworkSendFlags
	{
		NSF_unreliable		= 0,
		NSF_no_nagle		= 1,
		NSF_unreliable_no_nagle = NSF_unreliable | NSF_no_nagle,
		NSF_no_delay		= 4,
		NSF_unreliable_no_delay = NSF_unreliable | NSF_no_delay | NSF_no_nagle,
		NSF_reliable		= 8,
		NSF_reliable_no_nagle	= NSF_reliable | NSF_no_nagle,
		NSF_use_current_thread	= 16,
	};

	// Copy of ESteamNetworkingConnectionState
	enum NetworkConnectionState
	{
		NCS_none			= 0,
		NCS_connecting			= 1,
		NCS_finding_route		= 2,
		NCS_connected			= 3,
		NCS_closed_by_peer		= 4,
		NCS_problem_detected_locally	= 5,
	};

PUBLISHED:
	NetworkSystem();
	~NetworkSystem();

	NetworkConnectionHandle connect_by_IP_address( const NetAddress &addr );
	bool get_connection_info( NetworkConnectionHandle hConn, NetworkConnectionInfo *pInfo );
	void send_datagram( NetworkConnectionHandle hConn, const Datagram &dg,
			    NetworkSendFlags flags = NSF_reliable );
	void close_connection( NetworkConnectionHandle hConn );
	void run_callbacks( NetworkCallbacks *pCallbacks );
	bool accept_connection( NetworkConnectionHandle hConn );
	bool set_connection_poll_group( NetworkConnectionHandle hConn, NetworkPollGroupHandle hPollGroup );
	bool receive_message_on_connection( NetworkConnectionHandle hConn, NetworkMessage &msg );
	bool receive_message_on_poll_group( NetworkPollGroupHandle hPollGroup, NetworkMessage &msg );
	NetworkPollGroupHandle create_poll_group();
	NetworkListenSocketHandle create_listen_socket( int port );

PUBLISHED:
	static NetworkSystem *get_global_ptr();

private:
	ISteamNetworkingSockets *m_pInterface;
	static NetworkSystem *s_pGlobalPtr;
};

INLINE NetworkSystem *NetworkSystem::get_global_ptr()
{
	if ( !s_pGlobalPtr )
	{
		s_pGlobalPtr = new NetworkSystem;
	}

	return s_pGlobalPtr;
}

class EXPCL_NETWORKSYSTEM NetworkCallbacks : public ISteamNetworkingSocketsCallbacks
{
public:
	virtual void OnSteamNetConnectionStatusChanged( SteamNetConnectionStatusChangedCallback_t *pCallback ) override;

	virtual void on_connection_status_changed( NetworkConnectionHandle hConn,
						   NetworkSystem::NetworkConnectionState currState,
						   NetworkSystem::NetworkConnectionState oldState );

PUBLISHED:
	NetworkCallbacks();
	~NetworkCallbacks();

#ifdef HAVE_PYTHON
PUBLISHED:
	void set_callback( PyObject *pPyCallback );
private:
	PyObject *m_pPyCallback;
#endif
};

INLINE NetworkCallbacks::NetworkCallbacks()
{
#ifdef HAVE_PYTHON
	m_pPyCallback = nullptr;
#endif
}

INLINE NetworkCallbacks::~NetworkCallbacks()
{
#ifdef HAVE_PYTHON
	if ( m_pPyCallback )
	{
		Py_DECREF( m_pPyCallback );
		m_pPyCallback = nullptr;
	}
#endif
}

#ifdef HAVE_PYTHON
INLINE void NetworkCallbacks::set_callback( PyObject *pPyCallback )
{
	if ( m_pPyCallback )
	{
		Py_DECREF( m_pPyCallback );
	}

	m_pPyCallback = pPyCallback;
	Py_INCREF( m_pPyCallback );
}
#endif

class EXPCL_NETWORKSYSTEM NetworkConnectionInfo
{
PUBLISHED:
	NetworkListenSocketHandle listenSocket;
	NetAddress netAddress;
	NetworkSystem::NetworkConnectionState state;
	int endReason;
};

#endif // NETWORKSYSTEM_H