#pragma once

#include "config_clientdll.h"
#include "entityshared.h"
#include "simpleHashMap.h"
#include "c_baseentity.h"

class EXPORT_CLIENT_DLL ClientEntityManager
{
public:
	ClientEntityManager();

	void insert_entity( C_BaseEntity *ent );
	void remove_entity( entid_t entnum );
	void remove_entity( C_BaseEntity *ent );
	C_BaseEntity *get_entity( entid_t entnum ) const;
	C_BaseEntity *make_entity( const std::string &network_name, entid_t entnum );

public:
	SimpleHashMap<entid_t, PT( C_BaseEntity ), int_hash> edict;
};

INLINE ClientEntityManager::ClientEntityManager()
{
}

INLINE void ClientEntityManager::insert_entity( C_BaseEntity *ent )
{
	edict[ent->get_entnum()] = ent;
}

INLINE void ClientEntityManager::remove_entity( entid_t entnum )
{
	int ient = edict.find( entnum );
	if ( ient != -1 )
	{
		C_BaseEntity *ent = edict.get_data( ient );
		ent->despawn();
		edict.remove_element( ient );
	}
}

INLINE void ClientEntityManager::remove_entity( C_BaseEntity *ent )
{
	remove_entity( ent->get_entnum() );
}

INLINE C_BaseEntity *ClientEntityManager::get_entity( entid_t entnum ) const
{
	int ient = edict.find( entnum );
	if ( ient == -1 )
	{
		return nullptr;
	}

	return edict.get_data( ient );
}