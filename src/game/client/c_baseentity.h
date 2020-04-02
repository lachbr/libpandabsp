/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file c_baseentity.h
 * @author Brian Lach
 * @date January 25, 2020
 */

#ifndef C_BASEENTITY_H_
#define C_BASEENTITY_H_

#include <typedReferenceCount.h>

#include "config_clientdll.h"
#include "entityshared.h"
#include "baseentity_shared.h"
#include "client_class.h"
#include "interpolatedvar.h"

#include <aa_luse.h>
#include <nodePath.h>
#include <pvector.h>

NotifyCategoryDeclNoExport( c_baseentity )

class VarMapEntry_t
{
public:
	unsigned short type;
	unsigned short m_bNeedsToInterpolate; // Set to false when this var doesn't
	// need Interpolate() called on it anymore.
	void *data;
	IInterpolatedVar *watcher;
};

class VarMapping_t
{
public:
	VarMapping_t()
	{
		m_nInterpolatedEntries = 0;
		m_lastInterpolationTime = 0.0f;
	}

	pvector<VarMapEntry_t> m_Entries;
	int m_nInterpolatedEntries;
	float m_lastInterpolationTime;
};


class EXPORT_CLIENT_DLL C_BaseEntity : public CBaseEntityShared
{
	DECLARE_CLASS( C_BaseEntity, CBaseEntityShared );
	DECLARE_CLIENTCLASS();

public:
	C_BaseEntity();

	virtual bool is_predictable();
	virtual void post_data_update();
	virtual void update_parent_entity( int parent_entnum );
	virtual void spawn();
	virtual void init( entid_t entnum );
	virtual void post_interpolate();

	INLINE int get_parent_entity() const
	{
		return _parent_entity;
	}

	int interp_interpolate( VarMapping_t *map, float currentTime );
	void interp_setup_mappings( VarMapping_t *map );

	// Interpolate the position for rendering
	virtual bool interpolate( float currentTime );

	// Set appropriate flags and store off data when these fields are about to
	// change
	virtual void on_latch_interpolated_vars( int flags );
	// For predictable entities, stores last networked value
	void on_store_last_networked_value();

	float get_interpolate_amount( int flags );
	float get_last_changed_time( int flags );

	static void interpolate_entities();

protected:
	// overrideable rules if an entity should interpolate
	virtual bool should_interpolate();

	// Returns INTERPOLATE_STOP or INTERPOLATE_CONTINUE.
	// bNoMoreChanges is set to 1 if you can call RemoveFromInterpolationList on
	// the entity.
	int base_interpolate_1( float &currentTime, LVector3 &oldOrigin,
				  LVector3 &oldAngles, int &bNoMoreChanges );

	void add_to_interpolation_list();
	void remove_from_interpolation_list();

	void add_to_teleport_list();
	void remove_from_teleport_list();

public:
	void add_var( void *data, IInterpolatedVar *watcher, int type,
		     bool bSetup = false );
	void remove_var( void *data, bool bAssert = true );
	VarMapping_t *get_var_mapping();

	VarMapping_t _var_map;

	unsigned short _interpolation_list_entry;
	unsigned short _teleport_list_entry;

public:
	int _bsp_entnum;
	int _parent_entity;
	float _simulation_time;
	float _old_simulation_time;
	LVector3 _origin;
	LVector3 _angles;
	LVector3 _scale;
	CInterpolatedVar<LVector3> _iv_origin;
	CInterpolatedVar<LVector3> _iv_angles;
	CInterpolatedVar<LVector3> _iv_scale;
	int _simulation_tick;
};

#endif // BASEENTITY_H_
