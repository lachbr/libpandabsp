#pragma once

#include "config_serverdll.h"
#include "entityshared.h"
#include "netmessages.h"

#include <referenceCount.h>
#include <clockObject.h>
#include <pvector.h>

#include <steam/steamnetworkingsockets.h>

class CBasePlayer;

NotifyCategoryDeclNoExport( sv_client )

class EXPORT_SERVER_DLL Client : public ReferenceCount
{
public:
	INLINE Client( const SteamNetworkingIPAddr &addr, HSteamNetConnection socket, int id ) :
		_addr( addr ),
		_socket( socket ),
		_client_state( CLIENTSTATE_NONE ),
		_client_id( id ),
		_player( nullptr ),
		_frametime( 0.0f ),
		_tick( 0 )
	{
	}

	INLINE SteamNetworkingIPAddr get_addr() const
	{
		return _addr;
	}

	INLINE HSteamNetConnection get_connection() const
	{
		return _socket;
	}

	INLINE void set_client_state( int state )
	{
		_client_state = state;
	}

	INLINE int get_client_state() const
	{
		return _client_state;
	}

	INLINE int get_client_id() const
	{
		return _client_id;
	}

	INLINE void set_player( CBasePlayer *player )
	{
		_player = player;
	}

	INLINE CBasePlayer *get_player() const
	{
		return _player;
	}

	INLINE void set_tick( int tick, float frametime )
	{
		_tick = tick;
		_frametime = frametime;
	}

	INLINE int get_tick() const
	{
		return _tick;
	}

	INLINE float get_frametime() const
	{
		return _frametime;
	}

	void handle_cmd( DatagramIterator &dgi );

private:
	int _last_movement_tick;
	float _frametime;
	int _tick;
	int _client_state;
	int _client_id;
	SteamNetworkingIPAddr _addr;
	HSteamNetConnection _socket;
	CBasePlayer *_player;
};