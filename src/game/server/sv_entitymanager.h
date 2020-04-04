#pragma once

#include "baseentitysystem.h"
#include "uniqueIdAllocator.h"
#include "iserverdatagramhandler.h"

class CBaseEntity;

class ServerEntitySystem : public BaseEntitySystem, public IServerDatagramHandler
{
	DECLARE_CLASS( ServerEntitySystem, BaseEntitySystem )
public:
	ServerEntitySystem();

	virtual const char *get_name() const;

	virtual void update( double frametime );

	virtual bool initialize();
	
	virtual void remove_entity( entid_t entnum );
	virtual bool handle_datagram( Client *client, int msgtype, DatagramIterator &dgi );

	void build_snapshot( Datagram &dg, bool full_snapshot );

	CBaseEntity *make_entity_by_name( const std::string &name, bool spawn = true,
					  bool bexplicit_entnum = false, entid_t explicit_entnum = 0 );

	void run_entities( bool simulating );

private:
	UniqueIdAllocator _entnum_alloc;
};

INLINE const char *ServerEntitySystem::get_name() const
{
	return "ServerEntitySystem";
}

extern ServerEntitySystem *svents;