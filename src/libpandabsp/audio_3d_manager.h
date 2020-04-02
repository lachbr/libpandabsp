/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file audio_3d_manager.h
 * @author Brian Lach
 * @date July 05, 2019
 */

#ifndef AUDIO3DMANAGER_H
#define AUDIO3DMANAGER_H

#include "config_bsp.h"
#include <simpleHashMap.h>
#include <audioSound.h>
#include <nodePath.h>
#include <weakNodePath.h>
#include <genericAsyncTask.h>
#include <audioManager.h>
#include <weakPointerCallback.h>
#include <referenceCount.h>

struct nodeentry_t
{
	PandaNode *node;
	LPoint3 last_pos;
	pvector<PT( AudioSound )> sounds;
};

class EXPCL_PANDABSP Audio3DManager : public ReferenceCount
{
PUBLISHED:
	Audio3DManager( AudioManager *mgr, const NodePath &listener_target = NodePath(),
		const NodePath &root = NodePath() );

	INLINE void set_distance_factor( PN_stdfloat factor )
	{
		_mgr->audio_3d_set_distance_factor( factor );
	}
	INLINE PN_stdfloat get_distance_factor() const
	{
		return _mgr->audio_3d_get_distance_factor();
	}

	INLINE void set_doppler_factor( PN_stdfloat factor )
	{
		_mgr->audio_3d_set_doppler_factor( factor );
	}
	INLINE PN_stdfloat get_doppler_factor() const
	{
		return _mgr->audio_3d_get_doppler_factor();
	}

	INLINE void set_drop_off_factor( PN_stdfloat factor )
	{
		_mgr->audio_3d_set_drop_off_factor( factor );
	}
	INLINE PN_stdfloat get_drop_off_factor() const
	{
		return _mgr->audio_3d_get_drop_off_factor();
	}

	INLINE void set_sound_min_distance( AudioSound *sound, PN_stdfloat dist )
	{
		sound->set_3d_min_distance( dist );
	}
	INLINE PN_stdfloat get_sound_min_distance( AudioSound *sound ) const
	{
		return sound->get_3d_min_distance();
	}

	INLINE void set_sound_max_distance( AudioSound *sound, PN_stdfloat dist )
	{
		sound->set_3d_max_distance( dist );
	}
	INLINE PN_stdfloat get_sound_max_distance( AudioSound *sound ) const
	{
		return sound->get_3d_max_distance();
	}

	INLINE void attach_listener(const NodePath &listener)
	{
		_listener_target = listener;
		_listener_last_pos = _listener_target.get_pos( _root );
	}
	INLINE void detach_listener()
	{
		_listener_target = NodePath();
	}

	INLINE PT( AudioSound ) load_sfx( const std::string &path )
	{
		return _mgr->get_sound( path, true );
	}

	void attach_sound_to_object( AudioSound *sound, const NodePath &object );
	void detach_sound( AudioSound *sound );

	void print_audio_digest();

	void update();
	

private:
	PT( AudioManager ) _mgr;
	NodePath _listener_target;
	LPoint3 _listener_last_pos;
	NodePath _root;
	SimpleHashMap<PandaNode *, nodeentry_t, pointer_hash> _nodes;

	friend class Audio3DNodeWeakCallback;
};

#endif // AUDIO3DMANAGER_H
