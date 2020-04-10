#pragma once

#include "config_serverdll.h"
#include "baseentitysystem.h"
#include "uniqueIdAllocator.h"
#include "iserverdatagramhandler.h"

class CBaseEntity;

class EXPORT_SERVER_DLL ServerEntitySystem : public BaseEntitySystem, public IServerDatagramHandler
{
	DECLARE_CLASS( ServerEntitySystem, BaseEntitySystem )
public:
	ServerEntitySystem();

	virtual const char *get_name() const;

	virtual void update( double frametime );

	virtual bool initialize();
	
	virtual void remove_entity( entid_t entnum );
	virtual bool handle_datagram( Client *client, int msgtype, DatagramIterator &dgi );

	bool build_snapshot( Datagram &dg, uint32_t client_id, bool full_snapshot );

	CBaseEntity *make_entity_by_name( const std::string &name, bool spawn = true,
					  bool bexplicit_entnum = false, entid_t explicit_entnum = 0 );

	void run_entities( bool simulating );

	CBaseEntity *get_entity_from_name( const std::string &editorname ) const;
	void link_entity_to_class( const std::string &editorname, CBaseEntity *singleton );

	void reset_all_changed_props();

	static ServerEntitySystem *ptr();

private:
	UniqueIdAllocator _entnum_alloc;

	SimpleHashMap<std::string, CBaseEntity *, string_hash> _entity_to_class;
};

INLINE CBaseEntity *ServerEntitySystem::get_entity_from_name( const std::string &editorname ) const
{
	int ient = _entity_to_class.find( editorname );
	if ( ient != -1 )
	{
		return _entity_to_class.get_data( ient );
	}

	return nullptr;
}

INLINE void ServerEntitySystem::link_entity_to_class( const std::string &editorname, CBaseEntity *singleton )
{
	_entity_to_class[editorname] = singleton;
}

INLINE ServerEntitySystem *ServerEntitySystem::ptr()
{
	static PT( ServerEntitySystem ) global_ptr = new ServerEntitySystem;
	return global_ptr;
}

INLINE const char *ServerEntitySystem::get_name() const
{
	return "ServerEntitySystem";
}

extern ServerEntitySystem *svents;