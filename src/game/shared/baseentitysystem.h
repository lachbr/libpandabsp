#pragma once

#include "config_game_shared.h"
#include "entityshared.h"
#include "simpleHashMap.h"
#include "baseentity_shared.h"
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
	void remove_entity( entid_t entnum );
	void remove_entity( CBaseEntityShared *ent );
	size_t get_num_entities() const;
	CBaseEntityShared *get_entity( entid_t entnum ) const;

public:
	SimpleHashMap<entid_t, PT( CBaseEntityShared ), int_hash> edict;
};

INLINE BaseEntitySystem::BaseEntitySystem() :
	IGameSystem()
{
}

bool BaseEntitySystem::initialize()
{
}

void BaseEntitySystem::shutdown()
{
	size_t count = edict.size();
	for ( size_t i = 0; i < count; i++ )
	{
		edict.get_data( i )->despawn();
	}
	edict.clear();
}

void BaseEntitySystem::update( double frametime )
{
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