#pragma once

#include <datagramIterator.h>
#include <netAddress.h>
#include <referenceCount.h>
#include <steam/steamnetworkingsockets.h>

#include "client_commandmgr.h"
#include "iclientdatagramhandler.h"
#include "igamesystem.h"

class CUserCmd;

NotifyCategoryDeclNoExport( c_client )

class ClientNetInterface : public ISteamNetworkingSocketsCallbacks, public IGameSystem
{
	DECLARE_CLASS( ClientNetInterface, IGameSystem )
public:
	ClientNetInterface();

	virtual const char *get_name() const;
	virtual bool initialize();
	virtual void shutdown();
	virtual void update( double frametime );

	void add_datagram_handler( IClientDatagramHandler *handler );

	void set_client_state( int state );

	void send_datagram( Datagram &dg );

	void connect( const NetAddress &addr );

	virtual void OnSteamNetConnectionStatusChanged( SteamNetConnectionStatusChangedCallback_t *clbk ) override;

	INLINE int get_my_client_id() const
	{
		return _my_client_id;
	}

	INLINE float get_server_tickrate() const
	{
		return _server_tickrate;
	}

	void cmd_tick();

	void send_tick();

private:
	void read_incoming_messages();
	void handle_datagram( const Datagram &dg );
	void handle_server_tick( DatagramIterator &dgi );
	void connect_success( DatagramIterator &dgi );

public:
	pvector<IClientDatagramHandler *> _datagram_handlers;

	NetAddress _server_addr;

	HSteamNetConnection _connection;

	int _my_client_id;

	bool _connected;

	int _client_state;

	float _server_tickrate;

	float _last_server_tick_time;
	int _oldtickcount;
	int _servertickcount;
	float _tick_remainder;

	int _server_tick;
	float _server_frametime;

	CClientCMDManager _cmd_mgr;
};

INLINE const char *ClientNetInterface::get_name() const
{
	return "ClientNetSystem";
}

INLINE void ClientNetInterface::add_datagram_handler( IClientDatagramHandler *handler )
{
	_datagram_handlers.push_back( handler );
}

extern ClientNetInterface *clnet;