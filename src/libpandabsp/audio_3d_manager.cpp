/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file audio_3d_manager.cpp
 * @author Brian Lach
 * @date July 05, 2019
 */

#include "audio_3d_manager.h"

#include <asyncTaskManager.h>
#include <audioManager.h>
#include <clockObject.h>
#include <pStatCollector.h>
#include <pStatTimer.h>

struct audio3d_nodecallbackdata_t
{
	PandaNode *node;
	WeakReferenceList *list;
	Audio3DManager *mgr;
};

class Audio3DNodeWeakCallback : public WeakPointerCallback
{
public:
	virtual void wp_callback( void *pointer );
};

void Audio3DNodeWeakCallback::wp_callback( void *data )
{
	audio3d_nodecallbackdata_t *node_data = (audio3d_nodecallbackdata_t *)data;
	int itr = node_data->mgr->_nodes.find( node_data->node );
	if ( itr != -1 )
	{
		node_data->mgr->_nodes.remove( node_data->node );
	}
	
	delete data;
	delete this;
}

static PStatCollector attach_collector( "App:Audio3DManager:AttachSound" );
static PStatCollector detach_collector( "App:Audio3DManager:DetachSound" );
static PStatCollector update_collector( "App:Audio3DManager:Update" );

Audio3DManager::Audio3DManager( AudioManager *mgr, const NodePath &listener_target, const NodePath &root )
{
	_root = root;
	attach_listener( listener_target );
	_mgr = mgr;
}

void Audio3DManager::print_audio_digest()
{
	std::cout << _nodes.get_num_entries() << " nodes with sounds attached:" << std::endl;
	for ( size_t i = 0; i < _nodes.get_num_entries(); i++ )
	{
		NodePath np( _nodes.get_key( i ) );
		std::cout << "\t" << np;
		if ( np.get_top().get_name() != "render" )
		{
			std::cout << " ----- LEAKED";
		}
		std::cout << "\n";
	}
}

void Audio3DManager::attach_sound_to_object( AudioSound *sound, const NodePath &object )
{
	PStatTimer timer( attach_collector );

	PandaNode *node = object.node();

	int itr = _nodes.find( node );
	if ( itr == -1 )
	{
		nodeentry_t new_entry;
		new_entry.node = node;
		new_entry.last_pos = object.get_pos( _root );
		new_entry.sounds.push_back( sound );
		_nodes[node] = new_entry;

		// Add a callback to remove this node when the reference count
		// reaches zero.
		WeakReferenceList *ref = node->weak_ref();
		audio3d_nodecallbackdata_t *ncd = new audio3d_nodecallbackdata_t;
		ncd->mgr = this;
		ncd->node = new_entry.node;
		ncd->list = ref;
		ref->add_callback( new Audio3DNodeWeakCallback, ncd );
	}
	else
	{
		_nodes[node].sounds.push_back( sound );
	}
}

void Audio3DManager::detach_sound( AudioSound *sound )
{
}

void Audio3DManager::update()
{
	PStatTimer timer( update_collector );

	if ( !_mgr || !_mgr->get_active() )
	{
		return;
	}

	double dt = ClockObject::get_global_clock()->get_dt();

	for ( size_t i = 0; i < _nodes.get_num_entries(); i++ )
	{
		nodeentry_t &entry = _nodes[_nodes.get_key( i )];
		NodePath object = NodePath( entry.node );
		

		LPoint3 pos = object.get_pos( _root );
		object.clear();
		LVector3 vel = ( pos - entry.last_pos ) / dt;
		entry.last_pos = pos;

		for ( size_t j = 0; j < entry.sounds.size(); j++ )
		{
			AudioSound *sound = entry.sounds[j];
			sound->set_3d_attributes( pos[0], pos[1], pos[2], vel[0], vel[1], vel[2] );
		}
	}

	if ( !_listener_target.is_empty() )
	{
		LPoint3 pos = _listener_target.get_pos( _root );
		LVector3 fwd = _root.get_relative_vector( _listener_target, LVector3::forward() );
		LVector3 up = _root.get_relative_vector( _listener_target, LVector3::up() );
		
		LVector3 vel = ( pos - _listener_last_pos ) / dt;

		_listener_last_pos = pos;

		_mgr->audio_3d_set_listener_attributes( pos[0], pos[1], pos[2],
			vel[0], vel[1], vel[2],
			fwd[0], fwd[1], fwd[2],
			up[0], up[1], up[2] );
	}
	else
	{
		_mgr->audio_3d_set_listener_attributes( 0, 0, 0,
			0, 0, 0,
			0, 1, 0,
			0, 0, 1 );
	}
}
