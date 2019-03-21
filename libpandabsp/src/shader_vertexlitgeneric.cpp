/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file shader_vertexlitgeneric.cpp
 * @author Brian Lach
 * @date December 26, 2018
 */

#include "shader_vertexlitgeneric.h"

#include <auxBitplaneAttrib.h>
#include <transparencyAttrib.h>
#include <alphaTestAttrib.h>
#include <colorBlendAttrib.h>
#include <colorAttrib.h>
#include <renderState.h>
#include <materialAttrib.h>
#include <lightAttrib.h>
#include <clipPlaneAttrib.h>
#include <config_pgraphnodes.h>
#include <pointLight.h>
#include <directionalLight.h>
#include <spotlight.h>
#include <texturePool.h>

#include "bsploader.h"
#include "pssmCameraRig.h"
#include "bsp_material.h"
#include "shader_generator.h"

void VLGShaderConfig::parse_from_material_keyvalues( const BSPMaterial *mat )
{
        basetexture.parse_from_material_keyvalues( mat, this );
        alpha.parse_from_material_keyvalues( mat, this );
        arme.parse_from_material_keyvalues( mat, this );
        envmap.parse_from_material_keyvalues( mat, this );
        halflambert.parse_from_material_keyvalues( mat, this );
        detail.parse_from_material_keyvalues( mat, this );
        bumpmap.parse_from_material_keyvalues( mat, this );
        lightwarp.parse_from_material_keyvalues( mat, this );
        rimlight.parse_from_material_keyvalues( mat, this );
        selfillum.parse_from_material_keyvalues( mat, this );
}

VertexLitGenericSpec::VertexLitGenericSpec() :
        ShaderSpec( "VertexLitGeneric",
                    Filename( "phase_14/models/shaders/stdshaders/vertexLitGeneric_PBR.vert.glsl" ),
                    Filename( "phase_14/models/shaders/stdshaders/vertexLitGeneric_PBR.frag.glsl" ) )
{
}

PT( ShaderConfig ) VertexLitGenericSpec::make_new_config()
{
        return new VLGShaderConfig;
}

ShaderPermutations VertexLitGenericSpec::setup_permutations( const BSPMaterial *mat,
                                                             const RenderState *rs,
                                                             const GeomVertexAnimationSpec &anim,
                                                             PSSMShaderGenerator *generator )
{
        VLGShaderConfig *conf = (VLGShaderConfig *)get_shader_config( mat );

        ShaderPermutations result = ShaderSpec::setup_permutations( mat, rs, anim, generator );

        conf->basetexture.add_permutations( result );
        conf->alpha.add_permutations( result );
        conf->bumpmap.add_permutations( result );
        conf->detail.add_permutations( result );
        conf->arme.add_permutations( result );
        conf->envmap.add_permutations( result );
        conf->halflambert.add_permutations( result );
        conf->lightwarp.add_permutations( result );
        conf->rimlight.add_permutations( result );
        conf->selfillum.add_permutations( result );

        bool disable_alpha_write = false;
        bool calc_primary_alpha = false;

        bool need_tbn = false;
        bool need_world_position = false;
        bool need_world_normal = false;
        bool need_world_vec = false;
        bool need_eye_vec = true;
        bool need_eye_position = true;
        bool need_eye_normal = true;

        if ( conf->envmap.has_feature )
        {
                need_eye_vec = true;
                need_world_normal = true;
                need_world_position = true;
                need_world_vec = true;
                need_eye_normal = true;
        }
        if ( conf->bumpmap.has_feature )
        {
                need_tbn = true;
                need_eye_normal = true;
        }
        if ( conf->rimlight.has_feature )
        {
                need_world_normal = true;
                need_world_vec = true;
        }

        // verify_enforce_attrib_lock();
        const AuxBitplaneAttrib *aux_bitplane;
        rs->get_attrib_def( aux_bitplane );
        int outputs = aux_bitplane->get_outputs();

        // Decide whether or not we need alpha testing or alpha blending.
        bool have_alpha_test = false;
        bool have_alpha_blend = false;
        const AlphaTestAttrib *alpha_test;
        rs->get_attrib_def( alpha_test );
        if ( alpha_test->get_mode() != RenderAttrib::M_none &&
             alpha_test->get_mode() != RenderAttrib::M_always )
        {
                have_alpha_test = true;
        }
        const ColorBlendAttrib *color_blend;
        rs->get_attrib_def( color_blend );
        if ( color_blend->get_mode() != ColorBlendAttrib::M_none )
        {
                have_alpha_blend = true;
        }
        const TransparencyAttrib *transparency;
        rs->get_attrib_def( transparency );
        if ( transparency->get_mode() == TransparencyAttrib::M_alpha ||
             transparency->get_mode() == TransparencyAttrib::M_premultiplied_alpha ||
             transparency->get_mode() == TransparencyAttrib::M_dual ||
             conf->alpha.has_feature )
        {
                have_alpha_blend = true;
        }

        // Decide what to send to the framebuffer alpha, if anything.
        if ( outputs & AuxBitplaneAttrib::ABO_glow )
        {
                if ( have_alpha_blend )
                {
                        outputs &= ~AuxBitplaneAttrib::ABO_glow;
                        disable_alpha_write = true;
                        result.add_flag( ShaderAttrib::F_disable_alpha_write );
                }
                else if ( have_alpha_test )
                {
                        // Subsume the alpha test in our shader.
                        std::stringstream ss;
                        ss << alpha_test->get_mode();
                        result.add_permutation( "ALPHA_TEST", ss.str() );

                        std::stringstream rss;
                        rss << alpha_test->get_reference_alpha();
                        result.add_permutation( "ALPHA_TEST_REF", rss.str() );

                        result.add_flag( ShaderAttrib::F_subsume_alpha_test );
                }
        }

        if ( outputs & AuxBitplaneAttrib::ABO_aux_glow )
        {
                result.add_permutation( "HAVE_AUX_GLOW" );
        }
        if ( outputs & AuxBitplaneAttrib::ABO_aux_normal )
        {
                need_eye_normal = true;
                result.add_permutation( "HAVE_AUX_NORMAL" );
        }
        if ( outputs & AuxBitplaneAttrib::ABO_glow )
        {
                result.add_permutation( "HAVE_GLOW" );
        }

        if ( have_alpha_blend || have_alpha_test )
        {
                result.add_permutation( "CALC_PRIMARY_ALPHA" );
        }

        add_color( rs, result );

        BSPLoader *bsploader = BSPLoader::get_global_ptr();

        if ( !bsploader->has_active_level() )
        {
                // Break out the lights by type.
                const LightAttrib *la;
                rs->get_attrib_def( la );
                size_t num_lights = la->get_num_on_lights();
                PTA_int light_types = PTA_int::empty_array( num_lights );
                if ( num_lights > 0 )
                {
                        need_eye_normal = true;
                        need_eye_position = true;

                        result.add_permutation( "LIGHTING" );

                        std::stringstream ss;
                        ss << num_lights;
                        result.permutations["NUM_LIGHTS"] = ss.str();
                }
                for ( size_t i = 0; i < num_lights; i++ )
                {
                        TypeHandle type = DCAST( PandaNode, la->get_on_light( i ).node() )->get_class_type();
                        if ( type.is_derived_from( DirectionalLight::get_class_type() ) )
                        {
                                light_types[i] = 0;
                        }
                        else if ( type.is_derived_from( PointLight::get_class_type() ) )
                        {
                                light_types[i] = 2;
                        }
                        else if ( type.is_derived_from( Spotlight::get_class_type() ) )
                        {
                                light_types[i] = 3;
                        }
                }
                result.add_input( ShaderInput( "lightTypes", light_types ), false );
        }
        else
        {
                const LightAttrib *la;
                rs->get_attrib_def( la );

                // Make sure there is no setLightOff()
                if ( !la->has_all_off() )
                {
                        need_eye_normal = true;
                        need_eye_position = true;
                        need_world_normal = true; // for ambient cube

                        result.add_permutation( "LIGHTING" );
                        result.add_permutation( "BSP_LIGHTING" );
                        stringstream nlss;
                        nlss << MAX_TOTAL_LIGHTS;
                        result.add_permutation( "NUM_LIGHTS", nlss.str() );
                        result.add_permutation( "AMBIENT_CUBE" );
                }

        }

        if ( add_csm( result, generator ) )
        {
                need_world_normal = true;
                need_world_position = true;
        }

        const LightRampAttrib *lra;
        rs->get_attrib_def( lra );
        if ( lra->get_mode() != LightRampAttrib::LRT_default )
        {
                result.add_permutation( "HDR" );
        }

        const TextureAttrib *texture;
        rs->get_attrib_def( texture );

        size_t num_textures = texture->get_num_on_stages();
        for ( size_t i = 0; i < num_textures; ++i )
        {
                TextureStage *stage = texture->get_on_stage( i );
                Texture *tex = texture->get_on_texture( stage );
                nassertd( tex != nullptr ) continue;

                // Mark this TextureStage as having been used by the shader generator, so
                // that the next time its properties change, it will cause the state to be
                // rehashed to ensure that the shader is regenerated if needed.
                stage->mark_used_by_auto_shader();

                if ( stage->get_name() == "cubemap_tex" &&
                     !conf->envmap.envmap_texture &&
                     conf->envmap.has_feature)
                {
                        // env_cubemap (or something else) supplied envmap
                        result.add_input( ShaderInput( "envmapSampler", tex ) );
                }
        }

        if ( add_clip_planes( rs, result ) )
        {
                need_eye_position = true;
        }

        add_fog( rs, result );

        // Hardware skinning?
        if ( anim.get_animation_type() == GeomEnums::AT_hardware &&
             anim.get_num_transforms() > 0 )
        {
                result.permutations["HARDWARE_SKINNING"] = "1";
                int num_transforms;
                if ( anim.get_indexed_transforms() )
                {
                        num_transforms = 120;
                }
                else
                {
                        num_transforms = anim.get_num_transforms();
                }
                stringstream ss;
                ss << num_transforms;
                result.permutations["NUM_TRANSFORMS"] = ss.str();

                if ( anim.get_indexed_transforms() )
                {
                        result.permutations["INDEXED_TRANSFORMS"] = "1";
                }
        }

        if ( need_world_vec )
                need_world_position = true;

        if ( need_tbn )
        {
                result.add_permutation( "NEED_TBN" );
                result.add_permutation( "TANGENTNAME", "p3d_Tangent" );
                result.add_permutation( "BINORMALNAME", "p3d_Binormal" );
        }

        if ( need_eye_normal )
        {
                result.add_permutation( "NEED_EYE_NORMAL" );
        }
        if ( need_world_normal )
        {
                result.add_permutation( "NEED_WORLD_NORMAL" );
        }
        if ( need_world_position )
        {
                result.add_permutation( "NEED_WORLD_POSITION" );
        }
        if ( need_eye_position )
        {
                result.add_permutation( "NEED_EYE_POSITION" );
        }
        if ( need_eye_vec )
                result.add_permutation( "NEED_EYE_VEC" );
        if ( need_world_vec )
                result.add_permutation( "NEED_WORLD_VEC" );

        // Done!
        return result;
}