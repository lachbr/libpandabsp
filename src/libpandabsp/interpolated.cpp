/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file interpolated.cpp
 * @author Brian Lach
 * @date April 23, 2020
 */

#include "interpolated.h"

#include "configVariableDouble.h"

static ConfigVariableDouble interp_amount( "smooth-lag", 0.1 );

template <typename Type>
void CInterpolatedGroup::add_var( Type *data, IInterpolatedVar *watcher, int type )
{
	// Only add it if it hasn't been added yet.
	bool bAddIt = true;
	for ( size_t i = 0; i < _var_map.m_Entries.size(); i++ )
	{
		if ( _var_map.m_Entries[i].watcher == watcher )
		{
			if ( ( type & EXCLUDE_AUTO_INTERPOLATE ) !=
			     ( watcher->GetType() & EXCLUDE_AUTO_INTERPOLATE ) )
			{
				// Its interpolation mode changed, so get rid of it and re-add it.
				remove_var( _var_map.m_Entries[i].data, true );
			}
			else
			{
				// They're adding something that's already there. No need to re-add it.
				bAddIt = false;
			}

			break;
		}
	}

	if ( bAddIt )
	{
		// watchers must have a debug name set
		nassertv( watcher->GetDebugName() != NULL );

		VarMapEntry_t map;
		map.data = (void *)data;
		map.watcher = watcher;
		map.type = type;
		map.m_bNeedsToInterpolate = true;
		if ( type & EXCLUDE_AUTO_INTERPOLATE )
		{
			_var_map.m_Entries.push_back( map );
		}
		else
		{
			_var_map.m_Entries.insert( _var_map.m_Entries.begin(), map );
			++_var_map.m_nInterpolatedEntries;
		}
	}

	watcher->_Setup( (void *)data, type );
	watcher->SetInterpolationAmount( interp_amount );
}

template <typename Type>
void CInterpolatedGroup::remove_var( Type *data, bool assert )
{
	for ( size_t i = 0; i < _var_map.m_Entries.size(); i++ )
	{
		if ( _var_map.m_Entries[i].data == data )
		{
			if ( !( _var_map.m_Entries[i].type & EXCLUDE_AUTO_INTERPOLATE ) )
				--_var_map.m_nInterpolatedEntries;

			_var_map.m_Entries.erase( _var_map.m_Entries.begin() + i );
			return;
		}
	}
}

void CInterpolatedGroup::on_latch_interpolated_vars( int flags, float changetime )
{
	bool update_last = ( flags & INTERPOLATE_OMIT_UPDATE_LAST_NETWORKED ) == 0;

	size_t c = _var_map.m_Entries.size();
	for ( size_t i = 0; i < c; i++ )
	{
		VarMapEntry_t *e = &_var_map.m_Entries[i];
		IInterpolatedVar *watcher = e->watcher;

		int type = watcher->GetType();

		if ( !( type & flags ) )
			continue;

		if ( type & EXCLUDE_AUTO_LATCH )
			continue;

		if ( watcher->NoteChanged( changetime, update_last ) )
		{
			e->m_bNeedsToInterpolate = true;
		}

	}

	if ( _enabled )
	{
		_needs_interpolation = true;
	}
}

bool CInterpolatedGroup::interp_interpolate( VarMapping_t *map, float curr_time )
{
	bool done = true;
	if ( curr_time < map->m_lastInterpolationTime )
	{
		for ( int i = 0; i < map->m_nInterpolatedEntries; i++ )
		{
			VarMapEntry_t *e = &map->m_Entries[i];
			e->m_bNeedsToInterpolate = true;
		}
	}
	map->m_lastInterpolationTime = curr_time;

	for ( int i = 0; i < map->m_nInterpolatedEntries; i++ )
	{
		VarMapEntry_t *e = &map->m_Entries[i];

		if ( !e->m_bNeedsToInterpolate )
			continue;

		IInterpolatedVar *watcher = e->watcher;
		assert( !( watcher->GetType() & EXCLUDE_AUTO_INTERPOLATE ) );

		if ( watcher->Interpolate( curr_time ) )
			e->m_bNeedsToInterpolate = false;
		else
			done = false;
	}

	return done;
}
