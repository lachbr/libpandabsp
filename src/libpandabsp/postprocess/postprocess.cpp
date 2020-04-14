/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file postprocess.cpp
 * @author Brian Lach
 * @date July 22, 2019
 */

#include "postprocess/postprocess.h"
#include "postprocess/postprocess_defines.h"
#include "postprocess/postprocess_scene_pass.h"

#include "shader_generator.h"

#include <graphicsEngine.h>
#include <eventHandler.h>
#include <displayRegion.h>
#include <asyncTaskManager.h>
#include <auxBitplaneAttrib.h>

/////////////////////////////////////////////////////////////////////////////////////////
// PostProcess

PostProcess::PostProcess()
{
}

void PostProcess::startup( GraphicsOutput *output )
{
	_output = output;
	// Create the region that displays our final output image
	_output_display_region = _output->make_display_region();
	_output_display_region->set_sort( 0 );
	_output_display_region->disable_clears();

	_buffer_sort = _output->get_sort() - 1000;

	get_clears( _output, _window_clears );
	// We don't have to clear the window anymore.
	// The scene pass buffer will perform clearing.
	_output->disable_clears();

	int scene_flags = ( bits_PASSTEXTURE_COLOR |
			    bits_PASSTEXTURE_DEPTH |
			    //bits_PASSTEXTURE_AUX0 |
			    //bits_PASSTEXTURE_AUX1 |
			    bits_PASSTEXTURE_AUX2 );// |
			    //bits_PASSTEXTURE_AUX3 );

	_scene_pass = new PostProcessScenePass( this, scene_flags );
	_scene_pass->setup();
}

/**
 * Adds this camera to the postprocessing pipeline.
 * The scene rendered by this camera will be postprocessed.
 */
void PostProcess::add_camera( const NodePath &camera )
{
	PT( camerainfo_t ) info = new camerainfo_t;
	info->camera = camera;
	info->original_state = DCAST( Camera, camera.node() )->get_initial_state();
	info->state = info->original_state;

	DisplayRegion *region = nullptr;
	for ( int i = 0; i < _output->get_num_display_regions(); i++ )
	{
		DisplayRegion *r = _output->get_display_region( i );
		if ( r->get_camera() == camera )
		{
			region = r;
			break;
		}
	}
	if ( region != nullptr )
	{
		// Use the clears set on the old region for our new one.
		get_clears( region, info->region_clears );

		// If there is already a display region for this camera, get rid of it.
		// We will make our own display region for this camera to render into.
		_output->remove_display_region( region );
	}

	size_t i = _camera_info.size();
	_camera_info.push_back( info );

	_scene_pass->setup_scene_camera( i );
}

/**
 * Removes a camera that was once being used for postprocessing.
 * You should probably call this before removing your camera from the scene graph.
 */
void PostProcess::remove_camera( const NodePath &camera )
{
	PT( camerainfo_t ) info = nullptr;
	for ( size_t i = 0; i < _camera_info.size(); i++ )
	{
		if ( _camera_info[i]->camera == camera )
		{
			info = _camera_info[i];
			break;
		}
	}
	nassertv( info != nullptr );

	// Remove the display region that the camera was rendering into
	if ( info->new_region && info->new_region->get_window() )
	{
		info->new_region->get_window()->remove_display_region( info->new_region );
	}
	_camera_info.erase( std::find( _camera_info.begin(), _camera_info.end(), info ) );
}

void PostProcess::get_clears( DrawableRegion *region, PostProcess::ClearInfoArray &info )
{
	info.clear();
	info.resize( GraphicsOutput::RTP_COUNT );

	for ( int i = 0; i < GraphicsOutput::RTP_COUNT; i++ )
	{
		clearinfo_t cinfo;
		cinfo.active = region->get_clear_active( i );
		cinfo.value = region->get_clear_value( i );
		info[i] = cinfo;
	}
}

void PostProcess::set_clears( DrawableRegion *region, const PostProcess::ClearInfoArray &infoarray )
{
	for ( int i = 0; i < GraphicsOutput::RTP_COUNT; i++ )
	{
		const clearinfo_t &info = infoarray[i];
		region->set_clear_active( i, info.active );
		region->set_clear_value( i, info.value );
	}
}

void PostProcess::set_stacked_clears( DrawableRegion *region, const PostProcess::ClearInfoArray &a_array,
				      const PostProcess::ClearInfoArray &b_array )
{
	for ( int i = 0; i < GraphicsOutput::RTP_COUNT; i++ )
	{
		clearinfo_t info = b_array[i];
		if ( !info.active )
		{
			info = a_array[i];
		}
		region->set_clear_active( i, info.active );
		region->set_clear_value( i, info.value );
	}
}

void PostProcess::set_stacked_clears( int n, DrawableRegion *region )
{
	set_stacked_clears( region, _window_clears, _camera_info[n]->region_clears );
}

void PostProcess::set_scene_aux_bits( int bits )
{
	_scene_pass->set_camera_state( RenderState::make( AuxBitplaneAttrib::make( bits ) ) );
}

void PostProcess::shutdown()
{
	for ( size_t i = 0; i < _effects.size(); i++ )
	{
		_effects.get_data( i )->shutdown();
	}
	_effects.clear();

	_scene_pass->shutdown();
	_scene_pass = nullptr;

	_output = nullptr;

	for ( size_t i = 0; i < _camera_info.size(); i++ )
	{
		camerainfo_t *info = _camera_info[i];
		DCAST( Camera, info->camera.node() )->set_initial_state( info->original_state );
	}
	_camera_info.clear();
}

void PostProcess::update()
{
	size_t num_effects = _effects.size();
	for ( size_t i = 0; i < num_effects; i++ )
	{
		_effects.get_data( i )->update();
	}

	// Don't forget the main pass
	_scene_pass->update();
}

void PostProcess::window_event()
{
	// Something happened to the window.
	// Pass this along to all the effects.

	size_t num_effects = _effects.size();
	for ( size_t i = 0; i < num_effects; i++ )
	{
		_effects.get_data( i )->window_event( _output );
	}

	// Don't forget the main pass
	_scene_pass->window_event( _output );
}

void PostProcess::set_window_clears( DrawableRegion *region )
{
	set_clears( region, _window_clears );
}

void PostProcess::set_clears( int n, DrawableRegion *region )
{
	set_clears( region, _camera_info[n]->region_clears );
}
