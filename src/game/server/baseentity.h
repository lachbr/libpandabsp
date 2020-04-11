/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file baseentity.h
 * @author Brian Lach
 * @date January 24, 2020
 */

#ifndef BASEENTITY_H_
#define BASEENTITY_H_

#include <typedReferenceCount.h>

#include "config_serverdll.h"
#include "entityshared.h"
#include "networkvar.h"
#include "server_class.h"
#include "baseentity_shared.h"
#include "physicssystem.h"

#include <bitMask.h>

#include <aa_luse.h>

class CSceneComponent;
class CSimulationComponent;

struct entity_s;
typedef entity_s entity_t;

class EXPORT_SERVER_DLL CBaseEntity : public CBaseEntityShared
{
	DECLARE_CLASS( CBaseEntity, CBaseEntityShared )
public:
	CBaseEntity();

	virtual void add_components();

	virtual bool can_transition() const;

	virtual void init( entid_t entnum );
	virtual void spawn();

	void init_mapent( entity_t *ent, int bsp_entnum );

	INLINE bool has_entity_value( const std::string &key ) const
	{
		return _entity_keyvalues.find( key ) != -1;
	}

	INLINE std::string get_entity_value( const std::string &key ) const
	{
		int itr = _entity_keyvalues.find( key );
		if ( itr != -1 )
		{
			return _entity_keyvalues.get_data( itr );
		}
		return "";
	}

	LVector3 get_entity_value_vector( const std::string &key ) const;
	LColor get_entity_value_color( const std::string &key, bool scale = true ) const;

	INLINE std::string get_classname() const
	{
		return get_entity_value( "classname" );
	}
	INLINE std::string get_targetname() const
	{
		return get_entity_value( "targetname" );
	}

	INLINE int get_bsp_entnum() const
	{
		return _bsp_entnum;
	}

	virtual void receive_entity_message( int msgtype, uint32_t client_id, DatagramIterator &dgi );
	void send_entity_message( Datagram &dg );
	void send_entity_message( Datagram &dg, const vector_uint32 &client_ids );

	void set_owner_client( uint32_t client_id );
	uint32_t get_owner_client() const;

	void reset_changed_offsets();

	virtual PT( CBaseEntity ) make_new() = 0;

public:
	int _bsp_entnum;

	bool _map_entity;
	bool _preserved;

	CSceneComponent *_scene;
	CSimulationComponent *_sim;

	uint32_t _owner_client;

private:
	SimpleHashMap<std::string, std::string, string_hash> _entity_keyvalues;
};

INLINE void CBaseEntity::set_owner_client( uint32_t client_id )
{
	_owner_client = client_id;
}

INLINE uint32_t CBaseEntity::get_owner_client() const
{
	return _owner_client;
}

EXPORT_SERVER_DLL PT( CBaseEntity ) CreateEntityByName( const std::string &name );

#endif // BASEENTITY_H_
