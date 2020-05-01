/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file interpolated.h
 * @author Brian Lach
 * @date April 23, 2020
 */

#pragma once

#include "config_bsp.h"
#include "interpolatedvar.h"

#include "clockObject.h"

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

/**
 * This class manages and interpolates a group of interpolated variables.
 */
class EXPCL_PANDABSP CInterpolatedGroup
{
PUBLISHED:
	CInterpolatedGroup();

	void set_interpolation_enabled( bool enable );
	bool interpolation_enabled() const;

	//
	// Add/remove var mappings for interpolation.
	//
	template <typename Type>
	void add_var( Type *data, IInterpolatedVar *watcher, int type );
	template <typename Type>
	void remove_var( Type *data, bool assert = true );

	void add_float( float *data, IInterpolatedVar *watcher, int type );
	void add_vec2( LVector2f *data, IInterpolatedVar *watcher, int type );
	void add_vec3( LVector3f *data, IInterpolatedVar *watcher, int type );
	void add_vec4( LVector4f *data, IInterpolatedVar *watcher, int type );

	// Interpolate the variables
	void interpolate( float now );

	// Returns true if any of our variables are needing interpolation.
	bool needs_interpolation() const;

	void on_latch_interpolated_vars( int flags, float changetime );

private:
	bool interp_interpolate( VarMapping_t *map, float curr_time );

public:
	VarMapping_t *get_var_mapping();

private:
	bool _enabled;
	bool _needs_interpolation;
	VarMapping_t _var_map;
	ClockObject *_clock;
};

INLINE CInterpolatedGroup::CInterpolatedGroup() :
	_needs_interpolation( true ),
	_enabled( true ),
	_clock( ClockObject::get_global_clock() )
{
}

INLINE void CInterpolatedGroup::add_float( float *data, IInterpolatedVar *watcher, int type )
{
	add_var( data, watcher, type );
}

INLINE void CInterpolatedGroup::add_vec2( LVector2f *data, IInterpolatedVar *watcher, int type )
{
	add_var( data, watcher, type );
}

INLINE void CInterpolatedGroup::add_vec3( LVector3f *data, IInterpolatedVar *watcher, int type )
{
	add_var( data, watcher, type );
}

INLINE void CInterpolatedGroup::add_vec4( LVector4f *data, IInterpolatedVar *watcher, int type )
{
	add_var( data, watcher, type );
}

INLINE void CInterpolatedGroup::interpolate( float now )
{
	if ( !_needs_interpolation )
	{
		return;
	}

	_needs_interpolation = !interp_interpolate( get_var_mapping(), now );
}

INLINE void CInterpolatedGroup::set_interpolation_enabled( bool enabled )
{
	_enabled = enabled;
}

INLINE bool CInterpolatedGroup::interpolation_enabled() const
{
	return _enabled;
}

INLINE VarMapping_t *CInterpolatedGroup::get_var_mapping()
{
	return &_var_map;
}

/**
 * Returns true if any of our variables are needing interpolation.
 */
INLINE bool CInterpolatedGroup::needs_interpolation() const
{
	return _needs_interpolation;
}
