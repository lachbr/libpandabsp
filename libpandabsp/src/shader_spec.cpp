#include "shader_spec.h"
#include "shader_generator.h"

static ConfigVariableInt parallax_mapping_samples
( "parallax-mapping-samples", 3,
  PRC_DESC( "Sets the amount of samples to use in the parallax mapping "
            "implementation. A value of 0 means to disable it entirely." ) );

static ConfigVariableDouble parallax_mapping_scale
( "parallax-mapping-scale", 0.1,
  PRC_DESC( "Sets the strength of the effect of parallax mapping, that is, "
            "how much influence the height values have on the texture "
            "coordinates." ) );

TypeHandle ShaderSpec::_type_handle;

ShaderSpec::ShaderSpec( const std::string &name, const Filename &vert_file,
                        const Filename &pixel_file, const Filename &geom_file ) :
        ReferenceCount(),
        Namable( name ),
        _vertex_file( vert_file ),
        _pixel_file( pixel_file ),
        _geom_file( geom_file )
{
}

ShaderSpec::Permutations ShaderSpec::setup_permutations( const RenderState *state,
                                                         const GeomVertexAnimationSpec &anim, 
                                                         PSSMShaderGenerator *generator )
{
        Permutations result;
        return result;
}

//=================================================================================================

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

#include "bsploader.h"
#include "pssmCameraRig.h"

VertexLitGenericSpec::VertexLitGenericSpec() :
        ShaderSpec( "VertexLitGeneric",
                    Filename( "phase_14/models/shaders/stdshaders/vertexLitGeneric.vert.glsl" ),
                    Filename( "phase_14/models/shaders/stdshaders/vertexLitGeneric.frag.glsl" ) )
{
}

ShaderSpec::Permutations VertexLitGenericSpec::setup_permutations( const RenderState *rs,
                                                                   const GeomVertexAnimationSpec &anim, 
                                                                   PSSMShaderGenerator *generator )
{
        Permutations result = ShaderSpec::setup_permutations( rs, anim, generator );

        bool disable_alpha_write = false;
        bool calc_primary_alpha = false;

        bool need_tbn = false;
        bool need_world_position = false;
        bool need_eye_position = false;
        bool need_world_normal = false;
        bool need_eye_normal = false;

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
             transparency->get_mode() == TransparencyAttrib::M_dual )
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
                        result.flags.push_back( ShaderAttrib::F_disable_alpha_write );
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

                        result.flags.push_back( ShaderAttrib::F_subsume_alpha_test );
                }
        }

        if ( outputs & AuxBitplaneAttrib::ABO_aux_glow )
        {
                result.add_permutation( "HAVE_AUX_GLOW", "1" );
        }
        if ( outputs & AuxBitplaneAttrib::ABO_aux_normal )
        {
                need_eye_normal = true;
                result.add_permutation( "HAVE_AUX_NORMAL", "1" );
        }
        if ( outputs & AuxBitplaneAttrib::ABO_glow )
        {
                result.add_permutation( "HAVE_GLOW", "1" );
        }

        if ( have_alpha_blend || have_alpha_test )
        {
                result.add_permutation( "CALC_PRIMARY_ALPHA", "1" );
        }

        // Determine whether or not vertex colors or flat colors are present.
        const ColorAttrib *color;
        rs->get_attrib_def( color );
        ColorAttrib::Type ctype = color->get_color_type();
        if ( ctype != ColorAttrib::T_off )
        {
                result.add_permutation( "NEED_COLOR", "1" );
                if ( ctype == ColorAttrib::T_flat )
                {
                        result.add_permutation( "COLOR_FLAT", "1" );
                }
                else if ( ctype == ColorAttrib::T_vertex )
                {
                        result.add_permutation( "COLOR_VERTEX", "1" );
                }
        }

        // Store the material flags (not the material values itself).
        const MaterialAttrib *material;
        rs->get_attrib_def( material );
        Material *mat = material->get_material();
        if ( mat != nullptr )
        {
                mat->mark_used_by_auto_shader();
                int flags = mat->get_flags();
                if ( flags != 0 )
                {
                        result.add_permutation( "HAS_MAT", "1" );

                        if ( flags & Material::F_rim_color &&
                             flags & Material::F_rim_width )
                        {
                                result.permutations["MAT_RIM"] = "1";
                        }
                        if ( flags & Material::F_ambient )
                        {
                                result.permutations["MAT_AMBIENT"] = "1";
                        }
                        if ( flags & Material::F_diffuse )
                        {
                                result.permutations["MAT_DIFFUSE"] = "1";
                        }
                        if ( flags & Material::F_emission )
                        {
                                result.permutations["MAT_EMISSION"] = "1";
                        }
                        if ( flags & Material::F_lightwarp_texture )
                        {
                                result.permutations["MAT_LIGHTWARP"] = "1";
                        }
                        if ( mat->get_shade_model() == Material::SM_half_lambert )
                        {
                                result.permutations["MAT_HALFLAMBERT"] = "1";
                        }
                        if ( flags & Material::F_specular )
                        {
                                result.permutations["MAT_SPECULAR"] = "1";
                        }
                }
        }

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

                        result.permutations["LIGHTING"] = "1";

                        std::stringstream ss;
                        ss << num_lights;
                        result.permutations["NUM_LIGHTS"] = ss.str();
                        result.permutations["HAVE_SEPARATE_AMBIENT"] = "1";

                        if ( generator->has_shadow_sunlight() )
                        {
                                need_world_normal = true;

                                result.permutations["HAS_SHADOW_SUNLIGHT"] = "1";
                                result.permutations["PSSM_SPLITS"] = pssm_splits.get_string_value();
                                result.permutations["DEPTH_BIAS"] = depth_bias.get_string_value();
                                result.permutations["NORMAL_OFFSET_SCALE"] = normal_offset_scale.get_string_value();

                                stringstream ss;
                                ss << ( 1.0 / pssm_size ) * softness_factor;
                                result.permutations["SHADOW_BLUR"] = ss.str();

                                result.add_input( ShaderInput( "pssmSplitSampler", generator->get_pssm_array_texture() ) );
                                result.add_input( ShaderInput( "pssmMVPs", generator->get_pssm_rig()->get_mvp_array() ) );
                                result.add_input( ShaderInput( "sunVector", generator->get_pssm_rig()->get_sun_vector() ) );
                        }
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
                need_eye_normal = true;
                need_eye_position = true;
                need_world_normal = true; // for ambient cube

                result.permutations["LIGHTING"] = "1";
                result.permutations["BSP_LIGHTING"] = "1";
                stringstream nlss;
                nlss << MAXLIGHTS;
                result.permutations["NUM_LIGHTS"] = nlss.str();
                result.permutations["AMBIENT_CUBE"] = "1";
        }

        const LightRampAttrib *lra;
        rs->get_attrib_def( lra );
        if ( lra->get_mode() != LightRampAttrib::LRT_default )
        {
                result.permutations["HDR"] = "1";
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

                // In our system, the texture type is indicated by the name
                // of the stage in which the texture is applied on.
                // For instance, if a texture stage is named `normalmap`,
                // the texture applied on that stage will be treated
                // as a normal map.

                if ( stage == TextureStage::get_default() )
                {
                        result.permutations["ALBEDO"] = "1";
                        result.add_input( ShaderInput( "albedoSampler", tex ) );
                }
                if ( stage->get_name() == "normalmap" )
                {
                        result.permutations["NORMALMAP"] = "1";
                        result.add_input( ShaderInput( "normalSampler", tex ) );
                        need_tbn = true;
                        need_eye_normal = true;
                }
                else if ( stage->get_name() == "glowmap" )
                {
                        result.permutations["GLOWMAP"] = "1";
                        result.add_input( ShaderInput( "glowSampler", tex ) );
                }
                else if ( stage->get_name() == "glossmap" )
                {
                        result.permutations["GLOSSMAP"] = "1";
                        result.add_input( ShaderInput( "glossSampler", tex ) );
                }
                else if ( stage->get_name() == "heightmap" )
                {
                        result.permutations["HEIGHTMAP"] = "1";
                        result.permutations["PARALLAX_MAPPING_SCALE"] = parallax_mapping_scale.get_string_value();
                        result.permutations["PARALLAX_MAPPING_SAMPLES"] = parallax_mapping_samples.get_string_value();
                        result.add_input( ShaderInput( "heightSampler", tex ) );
                        need_tbn = true;
                }
                else if ( stage->get_name() == "spheremap" )
                {
                        result.permutations["SPHEREMAP"] = "1";
                        result.add_input( ShaderInput( "sphereSampler", tex ) );
                        need_tbn = true;
                        need_eye_position = true;
                        need_eye_normal = true;
                }
                else if ( stage->get_name() == "cubemap" )
                {
                        result.permutations["CUBEMAP"] = "1";
                        result.add_input( ShaderInput( "cubeSampler", tex ) );
                        need_eye_position = true;
                        need_eye_normal = true;
                        need_tbn = true;
                }
        }
        
        // Check for clip planes.
        const ClipPlaneAttrib *clip_plane;
        rs->get_attrib_def( clip_plane );
        stringstream ss;
        ss << clip_plane->get_num_on_planes();
        if ( clip_plane->get_num_on_planes() > 0 )
        {
                need_world_position = true;
        }
        result.permutations["NUM_CLIP_PLANES"] = ss.str();

        // Check for fog.
        const FogAttrib *fog;
        if ( rs->get_attrib( fog ) && !fog->is_off() )
        {
                stringstream fss;
                fss << (int)fog->get_fog()->get_mode();
                result.permutations["FOG"] = fss.str();
        }
        
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

        if ( need_tbn )
        {
                result.permutations["TANGENTNAME"] = "p3d_Tangent";
                result.permutations["BINORMALNAME"] = "p3d_Binormal";
        }

        if ( need_eye_normal )
        {
                result.permutations["NEED_EYE_NORMAL"] = "1";
        }
        if ( need_world_normal )
        {
                result.permutations["NEED_WORLD_NORMAL"] = "1";
        }
        if ( need_world_position )
        {
                result.permutations["NEED_WORLD_POSITION"] = "1";
        }
        if ( need_eye_position )
        {
                result.permutations["NEED_EYE_POSITION"] = "1";
        }

        // Done!
        return result;
}

LightmappedGenericSpec::LightmappedGenericSpec() :
        ShaderSpec( "LightmappedGeneric",
                    Filename( "phase_14/models/shaders/stdshaders/lightmappedGeneric.vert.glsl" ),
                    Filename( "phase_14/models/shaders/stdshaders/lightmappedGeneric.frag.glsl" ) )
{
}

ShaderSpec::Permutations LightmappedGenericSpec::setup_permutations( const RenderState *rs,
                                                                     const GeomVertexAnimationSpec &anim,
                                                                     PSSMShaderGenerator *generator )
{
        Permutations result = ShaderSpec::setup_permutations( rs, anim, generator );

        const TextureAttrib *tattr;
        rs->get_attrib_def( tattr );
        for ( size_t i = 0; i < tattr->get_num_on_stages(); i++ )
        {
                TextureStage *stage = tattr->get_on_stage( i );
                Texture *tex = tattr->get_on_texture( stage );

                if ( stage->get_name() == "basetexture" )
                {
                        result.add_permutation( "BASETEXTURE", "1" );
                        result.add_input( ShaderInput( "baseTextureSampler", tex ) );
                }
                else if ( stage->get_name() == "lightmap" )
                {
                        result.add_permutation( "FLAT_LIGHTMAP", "1" );
                        result.add_input( ShaderInput( "lightmapSampler", tex ) );
                }
                else if ( stage->get_name() == "lightmapArray" )
                {
                        result.add_permutation( "BUMPED_LIGHTMAP", "1" );
                        result.add_input( ShaderInput( "lightmapArraySampler", tex ) );
                }
                else if ( stage->get_name() == "normalmap" )
                {
                        result.add_permutation( "NORMALMAP", "1" );
                        result.add_input( ShaderInput( "normalSampler", tex ) );
                }
                else if ( stage->get_name() == "spheremap" )
                {
                        result.add_permutation( "SPHEREMAP", "1" );
                        result.add_input( ShaderInput( "sphereSampler", tex ) );
                        result.add_input( ShaderInput( "envmapShininess", LVecBase2( 1.0 ) ) );
                }
        }

        return result;
}