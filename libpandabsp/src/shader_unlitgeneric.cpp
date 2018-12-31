/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
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
                    Filename( "phase_14/models/shaders/stdshaders/unlitGeneric.vert.glsl" ),
                    Filename( "phase_14/models/shaders/stdshaders/unlitGeneric.frag.glsl" ) )
{
}

PT( ShaderConfig ) UnlitGenericSpec::make_new_config()
{
        return new ULGConfig;
}

ShaderPermutations UnlitGenericSpec::setup_permutations( const BSPMaterial *mat,
                                                         const RenderState *rs,
                                                         const GeomVertexAnimationSpec &anim,
                                                         PSSMShaderGenerator *generator )
{
        ULGConfig *conf = (ULGConfig *)get_shader_config( mat );
        ShaderPermutations result = ShaderSpec::setup_permutations( mat, rs, anim, generator );

        conf->basetexture.add_permutations( result );
        conf->alpha.add_permutations( result );

        add_fog( rs, result );

        return result;
}

