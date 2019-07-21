/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file shader_unlitnomat.cpp
 * @author Brian Lach
 * @date January 11, 2019
 */

#include "shader_unlitnomat.h"

#include <renderState.h>
#include <textureAttrib.h>

void UNMConfig::parse_from_material_keyvalues( const BSPMaterial *mat )
{
        // this shader doesn't use materials
}

UnlitNoMatSpec::UnlitNoMatSpec() :
        ShaderSpec( "UnlitNoMat",
                    Filename( "phase_14/models/shaders/stdshaders/unlitNoMat.vert.glsl" ),
                    Filename( "phase_14/models/shaders/stdshaders/unlitNoMat.frag.glsl" ) )
{
}

void UnlitNoMatSpec::setup_permutations( ShaderPermutations &result,
	const BSPMaterial *mat,
	const RenderState *rs,
	const GeomVertexAnimationSpec &anim,
	BSPShaderGenerator *generator )
{
	ShaderSpec::setup_permutations( result, mat, rs, anim, generator );

        // check for a single texture applied through setTexture()
        const TextureAttrib *ta;
        rs->get_attrib_def( ta );
        if ( ta->get_num_on_stages() > 0 &&
             ta->get_on_texture( ta->get_on_stage( 0 ) ) != nullptr )
        {
                // we have a texture
                result.add_permutation( "HAS_TEXTURE" );
        }

        // check for setColor()
        add_color( rs, result );

        add_hw_skinning( anim, result );
	add_alpha_test( rs, result );
	add_aux_bits( rs, result );
}

PT( ShaderConfig ) UnlitNoMatSpec::make_new_config()
{
        return new UNMConfig;
}