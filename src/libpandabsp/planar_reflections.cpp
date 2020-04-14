/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file planar_reflections.cpp
 * @author Brian Lach
 * @date November 23, 2019
 */

#include "planar_reflections.h"
#include <frameBufferProperties.h>
#include <cullFaceAttrib.h>
#include "shader_generator.h"

PlanarReflections::PlanarReflections( BSPShaderGenerator *shgen ) :
	_shgen( shgen ),
	_camera( nullptr ),
	_reflection_texture( nullptr ),
	_reflection_buffer( nullptr ),
	_is_setup( false )
{

	FrameBufferProperties fbp;
	fbp.clear();
	fbp.set_depth_bits( 8 );
	fbp.set_force_hardware( true );
	fbp.set_rgba_bits( 8, 8, 8, 8 );
	fbp.set_stencil_bits( 0 );
	fbp.set_float_color( false );
	fbp.set_srgb_color( false );
	fbp.set_float_depth( false );
	fbp.set_stereo( false );
	fbp.set_accum_bits( 0 );
	fbp.set_aux_float( 0 );
	fbp.set_aux_rgba( 0 );
	fbp.set_aux_hrgba( 0 );
	fbp.set_coverage_samples( 0 );
	fbp.set_multisamples( 0 );

	_reflection_buffer = _shgen->get_output()->
		make_texture_buffer( "planar-reflection", 1024, 1024, nullptr, false, &fbp );
	_reflection_texture = _reflection_buffer->get_texture();
	_reflection_texture->set_wrap_v( SamplerState::WM_clamp );
	_reflection_texture->set_wrap_u( SamplerState::WM_clamp );
	_reflection_buffer->set_sort( -100000 );
	_reflection_buffer->disable_clears();

	Camera* cam = DCAST( Camera, _shgen->get_camera().get_child( 0 ).node() );
	_camera = new Camera( "planar-reflection-camera", cam->get_lens() );
	_camera->set_camera_mask( CAMERA_REFLECTION );
	_camera_np = NodePath( _camera );

	PT( DisplayRegion ) dr = _reflection_buffer->make_display_region();
	dr->disable_clears();
	dr->set_clear_depth_active( true );
	dr->set_camera( _camera_np );
	dr->set_active( true );
}

void PlanarReflections::setup(const LVector3 &planevec, float height)
{
	if ( _is_setup )
		return;

	std::cout << "Setup planar reflections with ";
	planevec.output( std::cout );
	std::cout << ", " << height << std::endl;

	_height = height;
	_plane = LPlane( planevec, ( 0, 0, _height ) );
	_planenode = new PlaneNode( "planar-reflection-plane", _plane );
	NodePath plnp = _shgen->get_render().attach_new_node( _planenode );
	NodePath statenp( "statenp" );
	//statenp.set_clip_plane( plnp, 10 );
	statenp.set_attrib( CullFaceAttrib::make_reverse(), 10 );
	statenp.set_antialias( 0, 10 );
	_camera->set_initial_state( statenp.get_state() );

	_camera_np.reparent_to( _shgen->get_render() );

	_is_setup = true;
}

void PlanarReflections::shutdown()
{
	_camera_np.reparent_to( NodePath() );
	_is_setup = false;
}

void PlanarReflections::update()
{
	if ( _is_setup )
		_camera_np.set_mat( _shgen->get_camera().get_child( 0 ).get_mat( _shgen->get_render() ) * _plane.get_reflection_mat() );
}