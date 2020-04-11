#pragma once

#include "config_clientdll.h"
#include "basecomponent_shared.h"
#include "client_class.h"
#include "interpolatedvar.h"

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

class EXPORT_CLIENT_DLL C_BaseComponent : public BaseComponentShared
{
	DECLARE_CLASS( C_BaseComponent, BaseComponentShared )
public:
	C_BaseComponent();

	virtual bool initialize();
	virtual bool is_predictable();
	virtual void post_data_update()
	{
	}
	virtual void post_interpolate()
	{
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

	void add_var( void *data, IInterpolatedVar *watcher, int type,
		      bool bSetup = false );
	void remove_var( void *data, bool bAssert = true );
	VarMapping_t *get_var_mapping();

	VarMapping_t _var_map;

	unsigned short _interpolation_list_entry;
	unsigned short _teleport_list_entry;

	static void interpolate_components();

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
};