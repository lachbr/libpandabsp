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

#include <bulletRigidBodyNode.h>
#include <bitMask.h>

#include <aa_luse.h>

#define MAX_CHANGED_OFFSETS	20

struct entity_s;
typedef entity_s entity_t;

class EXPORT_SERVER_DLL CBaseEntity : public CBaseEntityShared
{
	DECLARE_CLASS( CBaseEntity, CBaseEntityShared );
	DECLARE_SERVERCLASS();
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
	BulletRigidBodyNode *get_physics_node() const;
	PT( BulletRigidBodyNode ) get_phys_body();

	virtual void transition_xform( const NodePath &landmark_np, const LMatrix4 &mat );

	virtual bool can_transition() const;

	virtual void init( entid_t entnum );
	virtual void spawn();
	virtual void despawn();

	void network_state_changed();
	void network_state_changed( void *ptr );

	bool is_property_changed( SendProp *prop );

	INLINE unsigned short *get_changed_offsets()
	{
		return _changed_offsets;
	}

	INLINE unsigned short get_change_offset( int i )
	{
		return _changed_offsets[i];
	}

	INLINE int get_num_changed_offsets() const
	{
		return _num_changed_offsets;
	}

	INLINE bool is_entity_fully_changed() const
	{
		return get_num_changed_offsets() >= MAX_CHANGED_OFFSETS;
	}

	INLINE void reset_changed_offsets()
	{
		_num_changed_offsets = 0;
	}

	INLINE int get_parent_entity() const
	{
		return _parent_entity;
	}

	INLINE void set_parent_entity( int ent )
	{
		_parent_entity = ent;
	}

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

	void set_origin( const LPoint3 &origin );
	void set_angles( const LVector3 &angles );
	void set_scale( const LVector3 &scale );

	virtual void receive_entity_message( int msgtype, uint32_t client_id, DatagramIterator &dgi );
	void send_entity_message( Datagram &dg );
	void send_entity_message( Datagram &dg, const vector_uint32 &client_ids );

	void update_network_time();

	virtual void simulate();

public:
	NetworkVar( int, _parent_entity );
	NetworkVec3( _origin );
	NetworkVec3( _angles );
	NetworkVec3( _scale );
	NetworkVar( int, _bsp_entnum );
	NetworkVar( float, _simulation_time );
	NetworkVar( int, _simulation_tick );

	// State
	int _state;
	float _state_time;

	// Physics things
	float _mass;
	SolidType _solid;
	bool _kinematic;
	bool _physics_setup;
	PT( BulletRigidBodyNode ) _bodynode;
	NodePath _bodynp;
	BitMask32 _phys_mask;

	LMatrix4f _landmark_relative_transform;
	bool _map_entity;
	bool _preserved;

	unsigned short _changed_offsets[MAX_CHANGED_OFFSETS];
	int _num_changed_offsets;

private:
	SimpleHashMap<std::string, std::string, string_hash> _entity_keyvalues;
};

INLINE BulletRigidBodyNode *CBaseEntity::get_physics_node() const
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
