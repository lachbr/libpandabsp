/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file postprocess.h
 * @author Brian Lach
 * @date July 22, 2019
 */

#ifndef POSTPROCESS_H
#define POSTPROCESS_H

#include "config_bsp.h"

#include <referenceCount.h>
#include <pointerTo.h>
#include <namable.h>
#include <simpleHashMap.h>
#include <nodePath.h>
#include <aa_luse.h>
#include <frameBufferProperties.h>
#include <graphicsBuffer.h>
#include <genericAsyncTask.h>
#include <camera.h>
#include <renderState.h>

#include "postprocess/postprocess_effect.h"
#include "postprocess/postprocess_scene_pass.h"

class GraphicsOutput;

class PostProcess
{
public:
	struct clearinfo_t
	{
		clearinfo_t()
		{
			active = false;
			value = LColor( 0 );
		}
		bool active;
		LColor value;
	};
	typedef pvector<clearinfo_t> ClearInfoArray;

	struct camerainfo_t : public ReferenceCount
	{
		camerainfo_t()
		{
			region_clears.resize( GraphicsOutput::RTP_COUNT );
			original_state = nullptr;
			state = nullptr;
			new_region = nullptr;
		}

		NodePath camera;
		CPT( RenderState ) original_state;
		CPT( RenderState ) state;
		ClearInfoArray region_clears;
		PT( DisplayRegion ) new_region;
	};
	typedef pvector<PT( camerainfo_t )> CameraInfoArray;

	INLINE size_t get_num_camera_infos() const
	{
		return _camera_info.size();
	}
	INLINE camerainfo_t *get_camera_info( int n )
	{
		return _camera_info[n];
	}

PUBLISHED:
	PostProcess( GraphicsOutput *output );

	void add_camera( const NodePath &camera );
	void remove_camera( const NodePath &camera );

	INLINE void add_effect( PostProcessEffect *effect )
	{
		_effects[effect->get_name()] = effect;
	}

	INLINE void remove_effect( PostProcessEffect *effect )
	{
		int itr = _effects.find( effect->get_name() );
		if ( itr != -1 )
			_effects.remove_element( itr );
	}

	INLINE PostProcessEffect *get_effect( const std::string &name )
	{
		return _effects[name];
	}

	INLINE Texture *get_scene_color_texture() const
	{
		return _scene_pass->get_texture( bits_PASSTEXTURE_COLOR );
	}
	INLINE Texture *get_scene_depth_texture() const
	{
		return _scene_pass->get_texture( bits_PASSTEXTURE_DEPTH );
	}

	INLINE PostProcessScenePass *get_scene_pass() const
	{
		return _scene_pass;
	}

	INLINE GraphicsOutput *get_output() const
	{
		return _output;
	}

	INLINE NodePath get_camera( int n ) const
	{
		return _camera_info[n]->camera;
	}

	INLINE int next_sort()
	{
		return _buffer_sort++;
	}

	INLINE DisplayRegion *get_output_display_region() const
	{
		return _output_display_region;
	}

	INLINE bool is_fullscreen() const
	{
		LVector4 dim = _output_display_region->get_dimensions();
		return ( dim[0] == 0.0f && dim[1] == 1.0f && dim[2] == 0.0f && dim[3] == 1.0f );
	}

	void set_scene_aux_bits( int bits );
	void set_stacked_clears( int n, DrawableRegion *region );
	void set_window_clears( DrawableRegion *region );
	void set_clears( int n, DrawableRegion *region );

	void shutdown();

private:
	static void handle_window_event( const Event *e, void *data );
	static AsyncTask::DoneStatus update_task( GenericAsyncTask *task, void *data );

private:
	ClearInfoArray _window_clears;
	CameraInfoArray _camera_info;

	void get_clears( DrawableRegion *region, ClearInfoArray &info );
	void set_clears( DrawableRegion *region, const ClearInfoArray &info );
	void set_stacked_clears( DrawableRegion *region, const ClearInfoArray &a, const ClearInfoArray &b );

	SimpleHashMap<std::string, PT( PostProcessEffect ), string_hash> _effects;
	GraphicsOutput *_output;

	PT( GenericAsyncTask ) _update_task;

	PT( DisplayRegion ) _output_display_region;

	// The main scene is rendered into the textures on this pass.
	// This is also the quad that gets displayed showing the final result
	// of all PostProcessEffects.
	PT( PostProcessScenePass ) _scene_pass;

	int _buffer_sort;
};

#endif // POSTPROCESS_H
