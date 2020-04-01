/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file bloom.cpp
 * @author Brian Lach
 * @date July 23, 2019
 */

#include "postprocess/bloom.h"
#include "postprocess/postprocess.h"
#include "postprocess/postprocess_pass.h"

#include <configVariableDouble.h>
#include <texture.h>

static ConfigVariableDouble r_bloomscale( "r_bloomscale", 1.0 );

static ConfigVariableDouble r_bloomtintr( "r_bloomtintr", 0.3 );
static ConfigVariableDouble r_bloomtintg( "r_bloomtintg", 0.59 );
static ConfigVariableDouble r_bloomtintb( "r_bloomtintb", 0.11 );
static ConfigVariableDouble r_bloomtintexponent( "r_bloomtintexponent", 2.2 );

class DownsampleLuminance : public PostProcessPass
{
public:
	DownsampleLuminance( PostProcess *pp ) :
		PostProcessPass( pp, "bloom-downsample_luminance", bits_PASSTEXTURE_COLOR ),
		_tap_offsets( PTA_LVecBase2f::empty_array( 4 ) )
	{
		// Downsample by 4
		set_div_size( true, 4 );
	}

	virtual void setup()
	{
		PostProcessPass::setup();
		get_quad().set_shader( Shader::load( Shader::SL_GLSL, "shaders/postprocess/downsample.vert.glsl",
						     "shaders/postprocess/downsample.frag.glsl" ) );

		// Vertex shader
		get_quad().set_shader_input( "tapOffsets", _tap_offsets );

		// Pixel shader
		get_quad().set_shader_input( "fbColorSampler", _pp->get_scene_pass()->get_texture( bits_PASSTEXTURE_AUX2 ) );
		get_quad().set_shader_input( "params", LVecBase4f( r_bloomtintr,
								   r_bloomtintg,
								   r_bloomtintb,
								   r_bloomtintexponent ) );
	}

	virtual void update()
	{
		PostProcessPass::update();

		// Size of the backbuffer/GraphicsWindow
		LVector2i bb_size = get_back_buffer_dimensions();
		float dx = 1.0f / bb_size[0];
		float dy = 1.0f / bb_size[1];

		_tap_offsets[0][0] = 0.5f * dx;
		_tap_offsets[0][1] = 0.5f * dy;

		_tap_offsets[1][0] = 2.5f * dx;
		_tap_offsets[1][1] = 0.5f * dy;

		_tap_offsets[2][0] = 0.5f * dx;
		_tap_offsets[2][1] = 2.5f * dy;

		_tap_offsets[3][0] = 2.5f * dx;
		_tap_offsets[3][1] = 2.5f * dy;
	}

private:
	PTA_LVecBase2f _tap_offsets;
};

class BlurX : public PostProcessPass
{
public:
	BlurX( PostProcess *pp, DownsampleLuminance *dsl ) :
		PostProcessPass( pp, "bloom_blurX", bits_PASSTEXTURE_COLOR ),
		_vs_tap_offsets( PTA_LVecBase2f::empty_array( 3 ) ),
		_ps_tap_offsets( PTA_LVecBase2f::empty_array( 3 ) ),
		_dsl( dsl )
	{
		set_div_size( true, 4 );
	}

	virtual void setup()
	{
		PostProcessPass::setup();
		get_quad().set_shader( Shader::load( Shader::SL_GLSL, "shaders/postprocess/blur.vert.glsl",
						     "shaders/postprocess/blur.frag.glsl" ) );
		get_quad().set_shader_input( "texSampler",
					     _dsl->get_color_texture() );
		get_quad().set_shader_input( "psTapOffsets", _ps_tap_offsets );
		get_quad().set_shader_input( "vsTapOffsets", _vs_tap_offsets );
		get_quad().set_shader_input( "scaleFactor", LVecBase3f( 1, 1, 1 ) );
	}

	virtual void update()
	{
		PostProcessPass::update();

		// Our buffer's size
		int width = _buffer->get_size()[0];
		float dx = 1.0f / width;

		_vs_tap_offsets[0][1] = 0.0f;
		_vs_tap_offsets[0][0] = 1.3366f * dx;

		_vs_tap_offsets[1][1] = 0.0f;
		_vs_tap_offsets[1][0] = 3.4295f * dx;

		_vs_tap_offsets[2][1] = 0.0f;
		_vs_tap_offsets[2][0] = 5.4264f * dx;

		/////////////////////////////////////////

		_ps_tap_offsets[0][1] = 0.0f;
		_ps_tap_offsets[0][0] = 7.4359f * dx;

		_ps_tap_offsets[1][1] = 0.0f;
		_ps_tap_offsets[1][0] = 9.4436f * dx;

		_ps_tap_offsets[2][1] = 0.0f;
		_ps_tap_offsets[2][0] = 11.4401f * dx;
	}

private:
	DownsampleLuminance *_dsl;

	PTA_LVecBase2f _vs_tap_offsets;
	PTA_LVecBase2f _ps_tap_offsets;
};

class BlurY : public PostProcessPass
{
public:
	BlurY( PostProcess *pp, BlurX *blur_x ) :
		PostProcessPass( pp, "bloom_blurY", bits_PASSTEXTURE_COLOR ),
		_vs_tap_offsets( PTA_LVecBase2f::empty_array( 3 ) ),
		_ps_tap_offsets( PTA_LVecBase2f::empty_array( 3 ) ),
		_blur_x( blur_x )
	{
		set_div_size( true, 4 );
	}

	virtual void setup()
	{
		PostProcessPass::setup();
		get_quad().set_shader( Shader::load( Shader::SL_GLSL, "shaders/postprocess/blur.vert.glsl",
						     "shaders/postprocess/blur.frag.glsl" ) );
		get_quad().set_shader_input( "texSampler",
					     _blur_x->get_color_texture() );
		get_quad().set_shader_input( "psTapOffsets", _ps_tap_offsets );
		get_quad().set_shader_input( "vsTapOffsets", _vs_tap_offsets );
		get_quad().set_shader_input( "scaleFactor", LVecBase3f( r_bloomscale ) );
	}

	virtual void update()
	{
		PostProcessPass::update();

		// Our buffer's size
		int height = _buffer->get_size()[1];
		float dy = 1.0f / height;

		_vs_tap_offsets[0][0] = 0.0f;
		_vs_tap_offsets[0][1] = 1.3366f * dy;

		_vs_tap_offsets[1][0] = 0.0f;
		_vs_tap_offsets[1][1] = 3.4295f * dy;

		_vs_tap_offsets[2][0] = 0.0f;
		_vs_tap_offsets[2][1] = 5.4264f * dy;

		/////////////////////////////////////////

		_ps_tap_offsets[0][0] = 0.0f;
		_ps_tap_offsets[0][1] = 7.4359f * dy;

		_ps_tap_offsets[1][0] = 0.0f;
		_ps_tap_offsets[1][1] = 9.4436f * dy;

		_ps_tap_offsets[2][0] = 0.0f;
		_ps_tap_offsets[2][1] = 11.4401f * dy;
	}

private:
	BlurX *_blur_x;

	PTA_LVecBase2f _vs_tap_offsets;
	PTA_LVecBase2f _ps_tap_offsets;
};

IMPLEMENT_CLASS( BloomEffect );

BloomEffect::BloomEffect( PostProcess *pp ) :
	PostProcessEffect( pp, "bloom" )
{
	// Downsample the framebuffer by 4, multiply image by luminance of image
	PT( DownsampleLuminance ) dsl = new DownsampleLuminance( pp );
	dsl->setup();

	// Separable gaussian blur
	PT( BlurX ) blur_x = new BlurX( pp, dsl );
	blur_x->setup();
	PT( BlurY ) blur_y = new BlurY( pp, blur_x );
	blur_y->setup();

	add_pass( dsl );
	add_pass( blur_x );
	add_pass( blur_y );
}

Texture *BloomEffect::get_final_texture()
{
	return get_pass( "bloom_blurY" )->get_color_texture();
}
