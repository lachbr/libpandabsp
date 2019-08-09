/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file postprocess_pass.cpp
 * @author Brian Lach
 * @date July 24, 2019
 */

#include "postprocess/postprocess_pass.h"
#include "postprocess/postprocess.h"

#include <graphicsEngine.h>
#include <cardMaker.h>
#include <orthographicLens.h>
#include <omniBoundingVolume.h>

/////////////////////////////////////////////////////////////////////////////////////////
// PostProcessPass

TypeDef( PostProcessPass );

FrameBufferProperties *PostProcessPass::_default_fbprops = nullptr;
FrameBufferProperties PostProcessPass::get_default_fbprops()
{
	if ( !_default_fbprops )
	{
		_default_fbprops = new FrameBufferProperties( FrameBufferProperties::get_default() );
		_default_fbprops->set_srgb_color( false );
		_default_fbprops->set_back_buffers( 0 );
		_default_fbprops->set_multisamples( 0 );
		_default_fbprops->set_accum_bits( 0 );
		_default_fbprops->set_aux_float( 0 );
		_default_fbprops->set_aux_rgba( 0 );
		_default_fbprops->set_aux_hrgba( 0 );
		_default_fbprops->set_coverage_samples( 0 );
		_default_fbprops->set_rgb_color( true );
		_default_fbprops->set_rgba_bits( 16, 16, 16, 8 );
	}

	return *_default_fbprops;
}

PostProcessPass::PostProcessPass( PostProcess *pp, const std::string &name, int output_texture_bits,
				  const FrameBufferProperties &fbprops,
				  bool force_size, const LVector2i &forced_size,
				  bool div_size, int div ) :
	Namable( name ),
	_pp( pp ),
	_output_texture_bits( output_texture_bits ),
	_fbprops( fbprops ),
	_force_size( force_size ),
	_forced_size( forced_size ),
	_buffer( nullptr ),
	_camera( nullptr ),
	_region( nullptr ),
	_div_size( div_size ),
	_div( div ),

	_color_texture( nullptr ),
	_depth_texture( nullptr ),
	_aux0_texture( nullptr ),
	_aux1_texture( nullptr ),
	_aux2_texture( nullptr ),
	_aux3_texture( nullptr )
{
}

LVector2i PostProcessPass::get_back_buffer_dimensions() const
{
	return _pp->get_output()->get_size();
}

LVector2i PostProcessPass::get_corrected_size( const LVector2i &size )
{
	if ( _force_size )
	{
		if ( _div_size )
		{
			return _forced_size / _div;
		}

		return _forced_size;
	}
	else if ( _div_size )
	{
		return size / _div;
	}

	return size;
}

bool PostProcessPass::setup_buffer()
{
	GraphicsOutput *window = _pp->get_output();

	WindowProperties winprops;
	winprops.set_size( get_corrected_size( window->get_size() ) );

	FrameBufferProperties fbprops = _fbprops;
	fbprops.set_back_buffers( 0 );
	if ( has_texture_bits( bits_PASSTEXTURE_DEPTH ) )
	{
		fbprops.set_depth_bits( 32 );
	}
	fbprops.set_stereo( window->is_stereo() );

	if ( has_texture_bits( bits_PASSTEXTURE_AUX0 ) )
	{
		fbprops.set_aux_rgba( 1 );
	}
	if ( has_texture_bits( bits_PASSTEXTURE_AUX1 ) )
	{
		fbprops.set_aux_rgba( 2 );
	}
	if ( has_texture_bits( bits_PASSTEXTURE_AUX2 ) )
	{
		fbprops.set_aux_rgba( 3 );
	}

	int flags = GraphicsPipe::BF_refuse_window;
	if ( !_force_size )
	{
		flags |= GraphicsPipe::BF_resizeable;
	}

	std::cout << "Setup buffer " << get_name() << std::endl;
	fbprops.output( std::cout );
	std::cout << std::endl;
	winprops.output( std::cout );
	std::cout << std::endl;
	std::cout << "Flags: " << flags << std::endl;

	PT( GraphicsOutput ) output = window->get_engine()->make_output(
		window->get_pipe(), get_name(), -1,
		fbprops, winprops, flags, window->get_gsg(),
		window );
	nassertr( output != nullptr, false );

	_buffer = DCAST( GraphicsBuffer, output );

	if ( has_texture_bits( bits_PASSTEXTURE_COLOR ) )
	{
		_buffer->add_render_texture( _color_texture, GraphicsOutput::RTM_bind_or_copy, GraphicsOutput::RTP_color );
	}
	if ( has_texture_bits( bits_PASSTEXTURE_DEPTH ) )
	{
		_buffer->add_render_texture( _depth_texture, GraphicsOutput::RTM_bind_or_copy, GraphicsOutput::RTP_depth );
	}
	if ( has_texture_bits( bits_PASSTEXTURE_AUX0 ) )
	{
		_buffer->add_render_texture( _aux0_texture, GraphicsOutput::RTM_bind_or_copy, GraphicsOutput::RTP_aux_rgba_0 );
	}
	if ( has_texture_bits( bits_PASSTEXTURE_AUX1 ) )
	{
		_buffer->add_render_texture( _aux1_texture, GraphicsOutput::RTM_bind_or_copy, GraphicsOutput::RTP_aux_rgba_1 );
	}
	if ( has_texture_bits( bits_PASSTEXTURE_AUX2 ) )
	{
		_buffer->add_render_texture( _aux2_texture, GraphicsOutput::RTM_bind_or_copy, GraphicsOutput::RTP_aux_rgba_2 );
	}
	if ( has_texture_bits( bits_PASSTEXTURE_AUX3 ) )
	{
		_buffer->add_render_texture( _aux3_texture, GraphicsOutput::RTM_bind_or_copy, GraphicsOutput::RTP_aux_rgba_3 );
	}

	_buffer->set_sort( _pp->next_sort() );
	_buffer->disable_clears();

	return true;
}

void PostProcessPass::setup_textures()
{
	if ( has_texture_bits( bits_PASSTEXTURE_COLOR ) )
		_color_texture = make_texture( get_name() + "-color" );

	if ( has_texture_bits( bits_PASSTEXTURE_DEPTH ) )
		_depth_texture = make_texture( get_name() + "-depth" );

	if ( has_texture_bits( bits_PASSTEXTURE_AUX0 ) )
		_aux0_texture = make_texture( get_name() + "-aux0" );

	if ( has_texture_bits( bits_PASSTEXTURE_AUX1 ) )
		_aux1_texture = make_texture( get_name() + "-aux1" );

	if ( has_texture_bits( bits_PASSTEXTURE_AUX2 ) )
		_aux2_texture = make_texture( get_name() + "-aux2" );

	if ( has_texture_bits( bits_PASSTEXTURE_AUX3 ) )
		_aux3_texture = make_texture( get_name() + "-aux3" );
}

void PostProcessPass::setup_quad()
{
	CardMaker cm( get_name() + "-quad" );
	cm.set_frame( -1, 1, -1, 1 );
	_quad_np = NodePath( cm.generate() );
	_quad_np.set_depth_test( false );
	_quad_np.set_depth_write( false );
}

void PostProcessPass::setup_camera()
{
	PT( OrthographicLens ) lens = new OrthographicLens;
	lens->set_film_size( 2, 2 );
	lens->set_film_offset( 0, 0 );
	lens->set_near_far( -1000, 1000 );

	PT( Camera ) cam = new Camera( get_name() + "-camera" );
	cam->set_bounds( new OmniBoundingVolume );
	cam->set_lens( lens );

	_camera = cam;
	_camera_np = _quad_np.attach_new_node( cam );
}

void PostProcessPass::setup_region()
{
	PT( DisplayRegion ) dr = _buffer->make_display_region( 0, 1, 0, 1 );
	dr->disable_clears();
	dr->set_camera( _camera_np );
	dr->set_active( true );
	dr->set_scissor_enabled( false );
	_region = dr;

	_buffer->set_clear_color( LColor( 0, 0, 0, 1 ) );
	_buffer->set_clear_color_active( true );
}

void PostProcessPass::setup()
{
	setup_textures();
	nassertv( setup_buffer() );
	setup_quad();
	setup_camera();
	setup_region();
}

void PostProcessPass::update()
{
}

void PostProcessPass::window_event( GraphicsOutput *output )
{
	if ( !_force_size && _buffer )
	{
		LVector2i size = output->get_size();
		get_corrected_size( size );
		_buffer->set_size( size[0], size[1] );
	}
}

PT( Texture ) PostProcessPass::make_texture( const std::string &name )
{
	PT( Texture ) tex = new Texture( name );
	tex->set_wrap_u( SamplerState::WM_clamp );
	tex->set_wrap_v( SamplerState::WM_clamp );
	return tex;
}

Texture *PostProcessPass::get_texture( int bit )
{
	if ( ( _output_texture_bits & bit ) == 0 )
	{
		return nullptr;
	}

	switch ( bit )
	{
	case bits_PASSTEXTURE_COLOR:
		return _color_texture;
	case bits_PASSTEXTURE_DEPTH:
		return _depth_texture;
	case bits_PASSTEXTURE_AUX0:
		return _aux0_texture;
	case bits_PASSTEXTURE_AUX1:
		return _aux1_texture;
	case bits_PASSTEXTURE_AUX2:
		return _aux2_texture;
	case bits_PASSTEXTURE_AUX3:
		return _aux3_texture;
	default:
		return nullptr;
	}
}

void PostProcessPass::shutdown()
{
	_buffer->remove_display_region( _region );
	_region = nullptr;
	_buffer->clear_render_textures();
	_buffer->get_engine()->remove_window( _buffer );
	_buffer = nullptr;
	_camera_np.remove_node();
	_camera = nullptr;
	_quad_np.remove_node();

	_color_texture = nullptr;
	_depth_texture = nullptr;
	_aux0_texture = nullptr;
	_aux1_texture = nullptr;
	_aux2_texture = nullptr;
	_aux3_texture = nullptr;

	_pp = nullptr;
}
