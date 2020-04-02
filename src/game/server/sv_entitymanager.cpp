#include "sv_entitymanager.h"
#include "baseentity.h"
#include "entregistry.h"

CBaseEntity *ServerEntityManager::make_entity_by_name( const std::string &name, bool spawn,
						       bool bexplicit_entnum, entid_t explicit_entnum )
{
	CBaseEntity *singleton = CEntRegistry::ptr()->_networkname_to_class[name];
	PT( CBaseEntity ) ent = singleton->make_new();
	if ( bexplicit_entnum )
		ent->init( explicit_entnum );
	else
		ent->init( _entnum_alloc.allocate() );
	ent->precache();

	if ( spawn )
		ent->spawn();

	insert_entity( ent );

	return ent;
}