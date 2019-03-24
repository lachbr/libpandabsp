/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file shader_lightmappedgeneric.cpp
 * @author Brian Lach
 * @date December 26, 2018
 */

#include "shader_lightmappedgeneric.h"
#include "bsploader.h"

#include <lightRampAttrib.h>
#include <textureStage.h>
#include <textureAttrib.h>
#include <renderState.h>

#include <string>

void LMGConfig::parse_from_material_keyvalues( const BSPMaterial *mat )
{
        basetexture.parse_from_material_keyvalues( mat, this );
        alpha.parse_from_material_keyvalues( mat, this );
        arme.parse_from_material_keyvalues( mat, this );
        envmap.parse_from_material_keyvalues( mat, this );
        bumpmap.parse_from_material_keyvalues( mat, this );
        detail.parse_from_material_keyvalues( mat, this );
}

LightmappedGenericSpec::LightmappedGenericSpec() :
        ShaderSpec( "LightmappedGeneric",
                    Filename( "phase_14/models/shaders/stdshaders/lightmappedGeneric_PBR.vert.glsl" ),
                    Filename( "phase_14/models/shaders/stdshaders/lightmappedGeneric_PBR.frag.glsl" ) )
{
}

string get_texcoord( int n )
{
        std::stringstream ss;
        ss << "p3d_MultiTexCoord" << n;
        return ss.str();
}

ShaderPermutations LightmappedGenericSpec::setup_permutations( const BSPMaterial *mat,
                                                               const RenderState *rs,
                                                               const GeomVertexAnimationSpec &anim,
                                                               BSPShaderGenerator *generator )
{
        LMGConfig *conf = (LMGConfig *)get_shader_config( mat );

        ShaderPermutations result = ShaderSpec::setup_permutations( mat, rs, anim, generator );

        const LightRampAttrib *lra;
        rs->get_attrib_def( lra );
        if ( lra->get_mode() != LightRampAttrib::LRT_default )
        {
                result.add_permutation( "HDR" );
        }

        conf->basetexture.add_permutations( result );
        conf->alpha.add_permutations( result );
        conf->arme.add_permutations( result );
        conf->envmap.add_permutations( result );
        conf->bumpmap.add_permutations( result );
        conf->detail.add_permutations( result );

        const TextureAttrib *tattr;
        rs->get_attrib_def( tattr );
        for ( size_t i = 0; i < tattr->get_num_on_stages(); i++ )
        {
                TextureStage *stage = tattr->get_on_stage( i );
                Texture *tex = tattr->get_on_texture( stage );

                if ( stage->get_name() == "lightmap" )
                {
                        result.add_permutation( "FLAT_LIGHTMAP" );
                        result.add_permutation( "TEXCOORD_LIGHTMAP", get_texcoord( i ) );
                        result.add_input( ShaderInput( "lightmapSampler", tex ) );
                }
                else if ( stage->get_name() == "lightmap_bumped" )
                {
                        result.add_permutation( "BUMPED_LIGHTMAP" );
                        result.add_permutation( "TEXCOORD_LIGHTMAP", get_texcoord( i ) );
                        result.add_input( ShaderInput( "lightmapSampler", tex ) );
                }

                // Check for an env_cubemap supplied cubemap_tex.
                else if ( stage->get_name() == "cubemap_tex" &&
                          !conf->envmap.envmap_texture &&
                          conf->envmap.has_feature )
                {
                        result.add_input( ShaderInput( "envmapSampler", tex ) );
                }
        }

        add_fog( rs, result );
        add_csm( result, generator );
        add_clip_planes( rs, result );

        return result;
}

PT( ShaderConfig ) LightmappedGenericSpec::make_new_config()
{
        return new LMGConfig;
}