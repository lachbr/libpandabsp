/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file shader_unlitgeneric.cpp
 * @author Brian Lach
 * @date December 30, 2018
 */

#include "shader_unlitgeneric.h"

void ULGConfig::parse_from_material_keyvalues( const BSPMaterial *mat )
{
        basetexture.parse_from_material_keyvalues( mat, this );
        alpha.parse_from_material_keyvalues( mat, this );
}

UnlitGenericSpec::UnlitGenericSpec() :
        ShaderSpec( "UnlitGeneric",
                    Filename( "shaders/stdshaders/unlitGeneric.vert.glsl" ),
                    Filename( "shaders/stdshaders/unlitGeneric.frag.glsl" ) )
{
}

PT( ShaderConfig ) UnlitGenericSpec::make_new_config()
{
        return new ULGConfig;
}

void UnlitGenericSpec::setup_permutations( ShaderPermutations &result,
	const BSPMaterial *mat,
	const RenderState *rs,
	const GeomVertexAnimationSpec &anim,
	BSPShaderGenerator *generator )
{
	ShaderSpec::setup_permutations( result, mat, rs, anim, generator );

        ULGConfig *conf = (ULGConfig *)get_shader_config( mat );

        conf->basetexture.add_permutations( result );
        conf->alpha.add_permutations( result );

        add_color( rs, result );
        add_fog( rs, result, generator );
        add_hw_skinning( anim, result );
	add_alpha_test( rs, result );
	add_aux_bits( rs, result );
}

