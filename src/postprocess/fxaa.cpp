/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file fxaa.cpp
 * @author Brian Lach
 * @date December 07, 2019
 */

#include "postprocess/fxaa.h"
#include "postprocess/postprocess_pass.h"
#include "postprocess/postprocess.h"

class FXAA_Pass : public PostProcessPass
{
public:
	FXAA_Pass( PostProcess *pp ) :
		PostProcessPass( pp, "fxaa-pass", bits_PASSTEXTURE_COLOR )
	{
	}

	virtual void setup()
	{
		PostProcessPass::setup();
		get_quad().set_shader(
			Shader::load(
				Shader::SL_GLSL,
				"shaders/postprocess/fxaa.vert.glsl",
				"shaders/postprocess/fxaa.frag.glsl"
			)
		);
		get_quad().set_shader_input( "sceneTexture", _pp->get_scene_color_texture() );
	}
};

TypeDef( FXAA_Effect );

FXAA_Effect::FXAA_Effect( PostProcess *pp ) :
	PostProcessEffect( pp, "fxaa" )
{
	PT( FXAA_Pass ) pass = new FXAA_Pass( pp );
	pass->setup();

	add_pass( pass );
}

Texture *FXAA_Effect::get_final_texture()
{
	return get_pass( "fxaa-pass" )->get_color_texture();
}