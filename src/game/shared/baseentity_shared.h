#pragma once

#include "config_game_shared.h"
#include "entityshared.h"
#include "entity_think.h"
#include "netmessages.h"
#include <simpleHashMap.h>
#include <nodePath.h>
#include <typedReferenceCount.h>
#include <datagram.h>

class EXPORT_GAME_SHARED CBaseEntityShared : public TypedReferenceCount
{
	DECLARE_CLASS( CBaseEntityShared, TypedReferenceCount )

public:
	CBaseEntityShared();

	INLINE NodePath get_node_path() const
	{
		return _np;
	}

	INLINE entid_t get_entnum() const
	{
		return _entnum;
	}

	virtual void init( entid_t entnum );
	virtual void precache();
	virtual void spawn();
	virtual void despawn();

	PT( CEntityThinkFunc ) make_think_func( const std::string &name, entitythinkfunc_t func, void *data, int sort = -1 );

	virtual void think();

	INLINE void set_next_think( double next )
	{
		_main_think->next_execute_time = next;
	}

	virtual void simulate();

	void run_thinks();

	void begin_entity_message( Datagram &dg, int msgtype );

private:
	static void think_func( CEntityThinkFunc *func, void *data );
	static void simulate_func( CEntityThinkFunc *func, void *data );

protected:
	bool _spawned;
	entid_t _entnum;
	NodePath _np;
	int _think_sort;

	CEntityThinkFunc *_main_think;
	pvector<PT( CEntityThinkFunc )> _thinks;
};

/**
 * Builds the header of an entity message.
 * Entity messages can be sent from a client to the server,
 * or from the server to client(s).
 */
INLINE void CBaseEntityShared::begin_entity_message( Datagram &dg, int msgtype )
{
	dg.add_uint16( NETMSG_ENTITY_MSG );
	dg.add_uint32( _entnum );
	dg.add_uint8( msgtype );
}