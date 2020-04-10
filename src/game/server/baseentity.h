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



struct entity_s;
typedef entity_s entity_t;

class EXPORT_SERVER_DLL CBaseEntity : public CBaseEntityShared
{
	DECLARE_CLASS( CBaseEntity, CBaseEntityShared )
public:
	enum SolidType
	{
		SOLID_BBOX,
		SOLID_MESH,
		SOLID_SPHERE,
		SOLID_NONE,
	};

	CBaseEntity();

	virtual void set_state( int state );

	int get_state() const;
	float get_state_elapsed() const;
	float get_state_time() const;

	// Physics methods
	void set_solid( SolidType type );
	void set_mass( float mass );
	void init_physics();
	CIBulletRigidBodyNode *get_physics_node() const;
	PT( CIBulletRigidBodyNode ) get_phys_body();

	virtual bool can_transition() const;

	virtual void init( entid_t entnum );
	virtual void spawn();
	virtual void despawn();

	void init_mapent( entity_t *ent, int bsp_entnum );

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

	void update_network_time();

	virtual void simulate();

	void set_owner_client( uint32_t client_id );
	uint32_t get_owner_client() const;

	void set_owner( entid_t entid );
	entid_t get_owner() const;

public:
	NetworkVar( entid_t, _owner_entity );
	int _bsp_entnum;

	// State
	int _state;
	float _state_time;

	// Physics things
	float _mass;
	SolidType _solid;
	bool _kinematic;
	bool _physics_setup;
	PT( CIBulletRigidBodyNode ) _bodynode;
	NodePath _bodynp;
	BitMask32 _phys_mask;

	bool _map_entity;
	bool _preserved;

	uint32_t _owner_client;

private:
	SimpleHashMap<std::string, std::string, string_hash> _entity_keyvalues;
};

INLINE void CBaseEntity::set_owner( entid_t entity )
{
	_owner_entity = entity;
}

INLINE entid_t CBaseEntity::get_owner() const
{
	return _owner_entity;
}

INLINE void CBaseEntity::set_owner_client( uint32_t client_id )
{
	_owner_client = client_id;
}

INLINE uint32_t CBaseEntity::get_owner_client() const
{
	return _owner_client;
}

INLINE CIBulletRigidBodyNode *CBaseEntity::get_physics_node() const
{
	return _bodynode;
}

INLINE void CBaseEntity::set_solid( SolidType type )
{
	_solid = type;
}

INLINE void CBaseEntity::set_mass( float mass )
{
	_mass = mass;
	if ( _mass > 0 )
	{
		_kinematic = true;
	}
}

INLINE int CBaseEntity::get_state() const
{
	return _state;
}

INLINE float CBaseEntity::get_state_time() const
{
	return _state_time;
}

INLINE float CBaseEntity::get_state_elapsed() const
{
	return _simulation_time - _state_time;
}

EXPORT_SERVER_DLL PT( CBaseEntity ) CreateEntityByName( const std::string &name );

#endif // BASEENTITY_H_
