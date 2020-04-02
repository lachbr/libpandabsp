#pragma once

#include <pmap.h>
#include <uniqueIdAllocator.h>
#include <referenceCount.h>

#include "baseentity.h"
#include "bsploader.h"
#include "client.h"
#include "iserverdatagramhandler.h"

#include <steam\steamnetworkingsockets.h>

NotifyCategoryDeclNoExport( server )

class EXPORT_SERVER_DLL ServerNetInterface : private ISteamNetworkingSocketsCallbacks, public ReferenceCount
{
public:
	ServerNetInterface();

	void add_datagram_handler( IServerDatagramHandler *handler );

	virtual PT( CBaseEntity ) make_player();

	bool startup();

	void broadcast_datagram( Datagram &dg, bool only_playing = false );

	void snapshot();
	void send_full_snapshot_to_client( Client *cl );
	void build_snapshot( Datagram &dg, bool full_snapshot );

	Datagram build_tick();
	void send_tick( Client *cl );

	void pre_frame_tick();
	void post_frame_tick();

	virtual void OnSteamNetConnectionStatusChanged( SteamNetConnectionStatusChangedCallback_t *clbk ) override;

	typedef pmap<HSteamNetConnection, PT( Client )> clientmap_t;

	void send_datagram( Datagram &dg, HSteamNetConnection conn );

	void process_cmd( Client *cl, const std::string &cmd );
	void update_client_state( Client *cl, int state );

	void read_incoming_messages();
	void run_networking_events();

	clientmap_t *get_clients();

private:

	int remove_client( Client *cl );

	void handle_datagram( Datagram &dg, Client *cl );

	void handle_client_hello( SteamNetConnectionStatusChangedCallback_t *info );

private:
	pvector<IServerDatagramHandler *> _datagram_handlers;

	UniqueIdAllocator _client_id_alloc;

	clientmap_t _clients_by_address;

	HSteamListenSocket _listen_socket;
	HSteamNetPollGroup _poll_group;

	bool _is_started;
};

INLINE void ServerNetInterface::add_datagram_handler( IServerDatagramHandler *handler )
{
	_datagram_handlers.push_back( handler );
}

INLINE ServerNetInterface::clientmap_t *ServerNetInterface::get_clients()
{
	return &_clients_by_address;
}