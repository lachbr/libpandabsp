/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file postprocess_scene_pass.cpp
 * @author Brian Lach
 * @date July 24, 2019
 */

#include "postprocess/postprocess_scene_pass.h"
#include "postprocess/postprocess.h"

#include <cardMaker.h>
#include <auxBitplaneAttrib.h>
#include <orthographicLens.h>

/////////////////////////////////////////////////////////////////////////////////////////
// PostProcessScenePass

PostProcessScenePass::PostProcessScenePass( PostProcess *pp, int output_texture_bits, int auxbits ) :
	PostProcessPass( pp, "scene_pass", output_texture_bits ),
	_aux_bits( auxbits ),
	_cam_state( nullptr )
{
}

bool PostProcessScenePass::setup_buffer()
{
	if ( !PostProcessPass::setup_buffer() )
		return false;

	_buffer->disable_clears();
	// Use the clears from the original window in our offscreen buffer.
	// The window is no longer performing clears.
	_pp->set_window_clears( _buffer );

	return true;
}

void PostProcessScenePass::setup_quad()
{
	CardMaker cm( get_name() + "-quad" );
	cm.set_frame_fullscreen_quad();
	_quad_np = NodePath( cm.generate() );
	_quad_np.set_depth_test( false );
	_quad_np.set_depth_write( false );
	if ( _color_texture )
	{
		_quad_np.set_texture( _color_texture );
	}
	_quad_np.set_color( 1.0f, 0.5f, 0.5, 1.0f );
}

void PostProcessScenePass::setup_camera()
{
	CPT( RenderState ) cam_state = RenderState::make_empty();
	if ( _aux_bits )
	{
		cam_state = cam_state->set_attrib( AuxBitplaneAttrib::make( _aux_bits ) );
	}
	set_camera_state( cam_state );

	PT( Camera ) cam = new Camera( get_name() + "-camera" );
	PT( OrthographicLens ) lens = new OrthographicLens;
	lens->set_film_size( 2, 2 );
	lens->set_film_offset( 0, 0 );
	lens->set_near_far( -1000, 1000 );
	cam->set_lens( lens );
	_camera_np = _quad_np.attach_new_node( cam );
	_camera = cam;
}

void PostProcessScenePass::set_camera_state( const RenderState *state )
{
	_cam_state = state;
	for ( size_t i = 0; i < _pp->get_num_camera_infos(); i++ )
	{
		PostProcess::camerainfo_t *info = _pp->get_camera_info( i );
		DCAST( Camera, info->camera.node() )->set_initial_state( _cam_state );
		info->state = _cam_state;
	}
}

/**
 * Sets up the scene camera to render its contents into our output textures.
 * This is used to have multiple cameras/display regions render into the same output.
 */
void PostProcessScenePass::setup_scene_camera( int i )
{
	PostProcess::camerainfo_t *info = _pp->get_camera_info( i );
	DCAST( Camera, info->camera.node() )->set_initial_state( _cam_state );
	info->state = _cam_state;

	PT( DisplayRegion ) dr = _buffer->make_display_region();
	dr->disable_clears();
	_pp->set_clears( i, dr );
	dr->set_camera( _pp->get_camera( i ) );
	dr->set_active( true );
	info->new_region = dr;
}

void PostProcessScenePass::setup_region()
{
	// Render into the final output display region
	_pp->get_output_display_region()->set_camera( _camera_np );
}
