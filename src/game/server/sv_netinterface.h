#pragma once

#include <pmap.h>
#include <uniqueIdAllocator.h>
#include <referenceCount.h>

#include "baseentity.h"
#include "bsploader.h"
#include "client.h"
#include "iserverdatagramhandler.h"
#include "igamesystem.h"

#include <steam\steamnetworkingsockets.h>

NotifyCategoryDeclNoExport( server )

class EXPORT_SERVER_DLL ServerNetInterface : private ISteamNetworkingSocketsCallbacks, public IGameSystem
{
	DECLARE_CLASS( ServerNetInterface, IGameSystem )

public:
	ServerNetInterface();

	// Methods of IGameSystem
	virtual const char *get_name() const;
	virtual bool initialize();
	virtual void shutdown();
	virtual void update( double frametime );

	void add_datagram_handler( IServerDatagramHandler *handler );

	virtual PT( CBaseEntity ) make_player();

	void send_tick();

	void broadcast_datagram( Datagram &dg, bool only_playing = false );

	void snapshot();
	void send_full_snapshot_to_client( Client *cl );

	Datagram build_tick();
	void send_tick( Client *cl );

	virtual void OnSteamNetConnectionStatusChanged( SteamNetConnectionStatusChangedCallback_t *clbk ) override;

	typedef pmap<HSteamNetConnection, PT( Client )> clientmap_t;

	void send_datagram( Datagram &dg, HSteamNetConnection conn );
	void send_datagram_to_clients( Datagram &dg, const vector_uint32 &client_ids );
	void send_datagram_to_client( Datagram &dg, uint32_t client_id );

	void process_cmd( Client *cl, const std::string &cmd );
	void update_client_state( Client *cl, int state );

	void read_incoming_messages();
	void run_networking_events();

	clientmap_t *get_clients();

	int get_max_clients() const;

private:

	int remove_client( Client *cl );

	void handle_datagram( Datagram &dg, Client *cl );

	void handle_client_hello( SteamNetConnectionStatusChangedCallback_t *info );

private:
	pvector<IServerDatagramHandler *> _datagram_handlers;

	UniqueIdAllocator _client_id_alloc;

	clientmap_t _clients_by_address;
	SimpleHashMap<uint32_t, Client *, int_hash> _clients_by_id;

	HSteamListenSocket _listen_socket;
	HSteamNetPollGroup _poll_group;

	int _max_clients;

	bool _is_started;
};

INLINE int ServerNetInterface::get_max_clients() const
{
	return _max_clients;
}

INLINE void ServerNetInterface::add_datagram_handler( IServerDatagramHandler *handler )
{
	_datagram_handlers.push_back( handler );
}

INLINE ServerNetInterface::clientmap_t *ServerNetInterface::get_clients()
{
	return &_clients_by_address;
}

INLINE const char *ServerNetInterface::get_name() const
{
	return "ServerNetInterface";
}

extern EXPORT_SERVER_DLL ServerNetInterface *svnet;