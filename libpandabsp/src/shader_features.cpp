/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file shader_features.cpp
 * @author Brian Lach
 * @date December 26, 2018
 */

#include "shader_features.h"
#include "bsp_material.h"
#include "vifparser.h"
#include "shader_spec.h"

#include <texturePool.h>

//==============================================================================================//

void PhongFeature::parse_from_material_keyvalues( const BSPMaterial *mat, ShaderConfig *conf )
{
        if ( mat->has_keyvalue( "$phong" ) )
        {
                if ( atoi( mat->get_keyvalue( "$phong" ).c_str() ) )
                {
                        conf->flags |= SF_PHONG;
                        has_feature = true;

                        if ( mat->has_keyvalue( "$phongexponent" ) )
                        {
                                phong_exponent = atof( mat->get_keyvalue( "$phongexponent" ).c_str() );
                        }
                        else if ( mat->has_keyvalue( "$phongexponenttexture" ) )
                        {
                                phong_exponent_texture = TexturePool::load_texture(
                                        mat->get_keyvalue( "$phongexponenttexture" )
                                );
                        }

                        if ( mat->has_keyvalue( "$phongboost" ) )
                        {
                                phong_boost = atof( mat->get_keyvalue( "$phongboost" ).c_str() );
                        }

                        if ( mat->has_keyvalue( "$phongfresnelranges" ) )
                        {
                                phong_fresnel_ranges = Parser::to_3f
                                ( mat->get_keyvalue( "$phongfresnelranges" ) );
                        }

                        if ( mat->has_keyvalue( "$phongalbedotint" ) )
                        {
                                phong_albedo_tint = (bool)
                                        atoi( mat->get_keyvalue( "$phongalbedotint" ).c_str() );
                        }

                        if ( mat->has_keyvalue( "$phongtint" ) )
                        {
                                phong_tint = Parser::to_3f
                                ( mat->get_keyvalue( "$phongtint" ) );
                        }

                        if ( mat->has_keyvalue( "$phongmask" ) )
                        {
                                phong_mask_texture = TexturePool::load_texture(
                                        mat->get_keyvalue( "$phongmask" )
                                );
                        }
                }
        }
}

void PhongFeature::add_permutations( ShaderPermutations &perms )
{
        if ( has_feature )
        {
                perms.add_permutation( "PHONG" );

                if ( phong_exponent_texture )
                {
                        perms.add_permutation( "PHONG_EXP_TEX" );
                        perms.add_input( ShaderInput( "phongExponentTexture", phong_exponent_texture ) );
                }
                else
                {
                        perms.add_input( ShaderInput( "phongExponent", LVector2( phong_exponent ) ) );
                }

                perms.add_input( ShaderInput( "phongBoost", LVector2( phong_boost ) ) );
                perms.add_input( ShaderInput( "phongFresnelRanges", phong_fresnel_ranges ) );
                
                if ( phong_albedo_tint )
                {
                        perms.add_permutation( "PHONG_ALBEDO_TINT" );
                }

                perms.add_input( ShaderInput( "phongTint", phong_tint ) );

                if ( phong_mask_texture )
                {
                        perms.add_permutation( "PHONG_MASK" );
                        perms.add_input( ShaderInput( "phongMaskSampler", phong_mask_texture ) );
                }
                
        }
}

//==============================================================================================//

void BaseTextureFeature::parse_from_material_keyvalues( const BSPMaterial *mat, ShaderConfig *conf )
{
        if ( mat->has_keyvalue( "$basetexture" ) )
        {
                conf->flags |= SF_BASETEXTURE;
                has_feature = true;

                if ( mat->has_keyvalue( "$basetexture_alpha" ) )
                {
                        // alpha channel supplied in a separate texture,
                        // common in Toontown
                        base_texture = TexturePool::load_texture( mat->get_keyvalue( "$basetexture" ),
                                                                  mat->get_keyvalue( "$basetexture_alpha" ) );
                }
                else
                {
                        base_texture = TexturePool::load_texture( mat->get_keyvalue( "$basetexture" ) );
                }

                
        }
}

void BaseTextureFeature::add_permutations( ShaderPermutations &perms )
{
        if ( has_feature && base_texture )
        {
                perms.add_permutation( "BASETEXTURE" );
                perms.add_input( ShaderInput( "baseTextureSampler", base_texture ) );
        }
}

//==============================================================================================//

void AlphaFeature::parse_from_material_keyvalues( const BSPMaterial *mat, ShaderConfig *conf )
{
        if ( mat->has_keyvalue( "$alpha" ) )
        {
                alpha = atof( mat->get_keyvalue( "$alpha" ).c_str() );

                has_feature = true;
                conf->flags |= SF_ALPHA;
        }
        else if ( mat->has_keyvalue( "$translucent" ) )
        {
                translucent = (bool)
                        atoi( mat->get_keyvalue( "$translucent" ).c_str() );

                has_feature = true;
                conf->flags |= SF_TRANSLUCENT;
        }
}

void AlphaFeature::add_permutations( ShaderPermutations &perms )
{
        if ( has_feature )
        {
                if ( !translucent && alpha != -1 )
                {
                        std::stringstream ss;
                        ss << alpha;
                        perms.add_permutation( "ALPHA", ss.str() );
                }
                else if ( translucent )
                {
                        perms.add_permutation( "TRANSLUCENT" );
                }
        }
}

//==============================================================================================//

void EnvmapFeature::parse_from_material_keyvalues( const BSPMaterial *mat, ShaderConfig *conf )
{
        if ( mat->has_keyvalue( "$envmap" ) )
        {
                conf->flags |= SF_ENVMAP;
                has_feature = true;

                std::string envmap = mat->get_keyvalue( "$envmap" );
                if ( envmap != "env_cubemap" )
                {
                        envmap_texture = TexturePool::load_cube_map( envmap );
                }

                if ( mat->has_keyvalue( "$envmapmask" ) )
                {
                        // controls per-pixel intensity of envmap contribution
                        envmap_mask_texture = TexturePool::load_texture( mat->get_keyvalue( "$envmapmask" ) );
                }

                if ( mat->has_keyvalue( "$envmaptint" ) )
                {
                        envmap_tint = Parser::to_3f( mat->get_keyvalue( "$envmaptint" ) );
                }

                if ( mat->has_keyvalue( "$envmapcontrast" ) )
                {
                        envmap_contrast = Parser::to_3f( mat->get_keyvalue( "$envmapcontrast" ) );
                }

                if ( mat->has_keyvalue( "$envmapsaturation" ) )
                {
                        envmap_saturation = Parser::to_3f( mat->get_keyvalue( "$envmapsaturation" ) );
                }
        }
        
}

void EnvmapFeature::add_permutations( ShaderPermutations &perms )
{
        if ( has_feature && ConfigVariableBool( "mat_envmaps", true ) )
        {
                perms.add_permutation( "ENVMAP" );

                // if no envmap texture,
                // we expect it to be filled in by the closest env_cubemap.
                if ( envmap_texture )
                {
                        perms.add_input( ShaderInput( "envmapSampler", envmap_texture ) );
                }

                if ( envmap_mask_texture )
                {
                        perms.add_permutation( "ENVMAP_MASK" );
                        perms.add_input( ShaderInput( "envmapMaskSampler", envmap_mask_texture ) );
                }

                perms.add_input( ShaderInput( "envmapTint", envmap_tint ) );
                perms.add_input( ShaderInput( "envmapContrast", envmap_contrast ) );
                perms.add_input( ShaderInput( "envmapSaturation", envmap_saturation ) );
        }
}

//==============================================================================================//

void DetailFeature::parse_from_material_keyvalues( const BSPMaterial *mat, ShaderConfig *conf )
{
        if ( mat->has_keyvalue( "$detail" ) )
        {
                conf->flags |= SF_DETAIL;
                has_feature = true;

                detail_texture = TexturePool::load_texture( mat->get_keyvalue( "$detail" ) );
                if ( mat->has_keyvalue( "$detailfactor" ) )
                {
                        detail_factor = atof( mat->get_keyvalue( "$detailfactor" ).c_str() );
                }
                if ( mat->has_keyvalue( "$detailscale" ) )
                {
                        detail_scale = atof( mat->get_keyvalue( "$detailscale" ).c_str() );
                }
                if ( mat->has_keyvalue( "$detailtint" ) )
                {
                        detail_tint = Parser::to_3f
                        ( mat->get_keyvalue( "$detailtint" ) );
                }
        }  
}

void DetailFeature::add_permutations( ShaderPermutations &perms )
{
        if ( has_feature && detail_texture )
        {
                perms.add_permutation( "DETAIL" );
                perms.add_input( ShaderInput( "detailSampler", detail_texture ) );

                perms.add_input( ShaderInput( "detailParams",
                                              LVector2( detail_factor, detail_scale ) ) );
                perms.add_input( ShaderInput( "detailTint", detail_tint ) );
        }
}

//==============================================================================================//

void HalfLambertFeature::parse_from_material_keyvalues( const BSPMaterial *mat, ShaderConfig *conf )
{
        // it's on by default, unless turned off

        bool off = false;

        if ( mat->has_keyvalue( "$halflambert" ) )
        {
                if ( atoi( mat->get_keyvalue( "$halflambert" ).c_str() ) )
                {
                        has_feature = true;
                        halflambert = true;
                }
                else
                {
                        has_feature = false;
                        halflambert = false;
                        off = true;
                }
        }

        if ( !off )
                conf->flags |= SF_HALFLAMBERT;
}

void HalfLambertFeature::add_permutations( ShaderPermutations &perms )
{
        if ( has_feature && halflambert )
        {
                perms.add_permutation( "HALFLAMBERT" );
        }
}

//==============================================================================================//

void BumpmapFeature::parse_from_material_keyvalues( const BSPMaterial *mat, ShaderConfig *conf )
{
        if ( mat->has_keyvalue( "$bumpmap" ) )
        {
                has_feature = true;
                conf->flags |= SF_BUMPMAP;

                bump_tex = TexturePool::load_texture( mat->get_keyvalue( "$bumpmap" ) );
        }
}

void BumpmapFeature::add_permutations( ShaderPermutations &perms )
{
        if ( has_feature && bump_tex )
        {
                perms.add_permutation( "BUMPMAP" );
                perms.add_input( ShaderInput( "bumpSampler", bump_tex ) );
        }
}

//==============================================================================================//

void LightwarpFeature::parse_from_material_keyvalues( const BSPMaterial *mat, ShaderConfig *conf )
{
        if ( mat->has_keyvalue( "$lightwarp" ) )
        {
                has_feature = true;
                conf->flags |= SF_LIGHTWARP;

                lightwarp_tex = TexturePool::load_texture( mat->get_keyvalue( "$lightwarp" ) );
        }
}

void LightwarpFeature::add_permutations( ShaderPermutations &perms )
{
        if ( has_feature && lightwarp_tex )
        {
                perms.add_permutation( "LIGHTWARP" );
                perms.add_input( ShaderInput( "lightwarpSampler", lightwarp_tex ) );
        }
}

//==============================================================================================//