#pragma once

#include "config_game_shared.h"
#include "entityshared.h"
#include "simpleHashMap.h"
#include "baseentity_shared.h"
#include "basecomponent_shared.h"
#include "igamesystem.h"

class EXPORT_GAME_SHARED BaseEntitySystem : public IGameSystem
{
	DECLARE_CLASS( BaseEntitySystem, IGameSystem )

public:
	BaseEntitySystem();

	virtual bool initialize();
	virtual void shutdown();
	virtual void update( double frametime );

	void insert_entity( CBaseEntityShared *ent );
	virtual void remove_entity( entid_t entnum );
	void remove_entity( CBaseEntityShared *ent );
	size_t get_num_entities() const;
	CBaseEntityShared *get_entity( entid_t entnum ) const;

	void register_component( BaseComponentShared *singleton );
	BaseComponentShared *get_component( componentid_t id ) const;

public:
	SimpleHashMap<entid_t, PT( CBaseEntityShared ), integer_hash<entid_t>> edict;
	SimpleHashMap<componentid_t, PT( BaseComponentShared ), integer_hash<componentid_t>> component_registry;
};

INLINE BaseEntitySystem::BaseEntitySystem() :
	IGameSystem()
{
}

INLINE BaseComponentShared *BaseEntitySystem::get_component( componentid_t id ) const
{
	int isingle = component_registry.find( id );
	if ( isingle != -1 )
	{
		return component_registry.get_data( isingle );
	}

	return nullptr;
}

INLINE void BaseEntitySystem::insert_entity( CBaseEntityShared *ent )
{
	edict[ent->get_entnum()] = ent;
}

INLINE void BaseEntitySystem::remove_entity( entid_t entnum )
{
	int ient = edict.find( entnum );
	if ( ient != -1 )
	{
		CBaseEntityShared *ent = edict.get_data( ient );
		ent->despawn();
		edict.remove_element( ient );
	}
}

INLINE void BaseEntitySystem::remove_entity( CBaseEntityShared *ent )
{
	remove_entity( ent->get_entnum() );
}

INLINE size_t BaseEntitySystem::get_num_entities() const
{
	return edict.size();
}

INLINE CBaseEntityShared *BaseEntitySystem::get_entity( entid_t entnum ) const
{
	int ient = edict.find( entnum );
	if ( ient == -1 )
	{
		return nullptr;
	}

	return edict.get_data( ient );
}