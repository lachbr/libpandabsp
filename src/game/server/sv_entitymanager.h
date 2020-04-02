#pragma once

#include "baseentitymanager.h"
#include "uniqueIdAllocator.h"
#include "iserverdatagramhandler.h"

class CBaseEntity;

class ServerEntityManager : public BaseEntityManager, public IServerDatagramHandler
{
public:
	ServerEntityManager();

	virtual bool handle_datagram( Client *client, int msgtype, DatagramIterator &dgi );

	CBaseEntity *make_entity_by_name( const std::string &name, bool spawn = true,
					  bool bexplicit_entnum = false, entid_t explicit_entnum = 0 );
private:
	UniqueIdAllocator _entnum_alloc;
};

INLINE ServerEntityManager::ServerEntityManager() :
	_entnum_alloc( ENTID_MIN + 1, ENTID_MAX )
{

}