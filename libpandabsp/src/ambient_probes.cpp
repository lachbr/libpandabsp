#include "ambient_probes.h"
#include "bsploader.h"
#include "bspfile.h"
#include "mathlib.h"
#include "bsptools.h"
#include "winding.h"

#include <shader.h>
#include <loader.h>
#include <textNode.h>
#include <pStatCollector.h>
#include <clockObject.h>
#include <material.h>
#include <auxBitplaneAttrib.h>
#include <alphaTestAttrib.h>
#include <colorBlendAttrib.h>
#include <materialAttrib.h>
#include <clipPlaneAttrib.h>
#include <virtualFileSystem.h>
#include <modelNode.h>
#include <pstatTimer.h>

ConfigVariableInt parallax_mapping_samples
( "parallax-mapping-samples", 3,
  PRC_DESC( "Sets the amount of samples to use in the parallax mapping "
            "implementation. A value of 0 means to disable it entirely." ) );

ConfigVariableDouble parallax_mapping_scale
( "parallax-mapping-scale", 0.1,
  PRC_DESC( "Sets the strength of the effect of parallax mapping, that is, "
            "how much influence the height values have on the texture "
            "coordinates." ) );

#include "bsp_trace.h"

#define PACK_COMBINE(src0, op0, src1, op1, src2, op2) ( \
  ((uint16_t)src0) | ((((uint16_t)op0 - 1u) & 3u) << 3u) | \
  ((uint16_t)src1 << 5u) | ((((uint16_t)op1 - 1u) & 3u) << 8u) | \
  ((uint16_t)src2 << 10u) | ((((uint16_t)op2 - 1u) & 3u) << 13u))

#define UNPACK_COMBINE_SRC(from, n) (TextureStage::CombineSource)((from >> ((uint16_t)n * 5u)) & 7u)
#define UNPACK_COMBINE_OP(from, n) (TextureStage::CombineOperand)(((from >> (((uint16_t)n * 5u) + 3u)) & 3u) + 1u)

BSPShaderGenerator::shaderinfo_t::shaderinfo_t() :
        color_type( ColorAttrib::T_vertex ),
        material_flags( 0 ),
        texture_flags( 0 ),
        fog_mode( 0 ),
        outputs( 0 ),
        calc_primary_alpha( false ),
        disable_alpha_write( false ),
        alpha_test_mode( RenderAttrib::M_none ),
        alpha_test_ref( 0.0 ),
        num_clip_planes( 0 ),
        light_ramp( nullptr ),
        shade_model( Material::SM_lambert )
{
}

bool BSPShaderGenerator::shaderinfo_t::operator<( const shaderinfo_t &other ) const
{
        if ( anim_spec != other.anim_spec )
        {
                return anim_spec < other.anim_spec;
        }
        if ( shade_model != other.shade_model )
        {
                return shade_model < other.shade_model;
        }
        if ( color_type != other.color_type )
        {
                return color_type < other.color_type;
        }
        if ( material_flags != other.material_flags )
        {
                return material_flags < other.material_flags;
        }
        if ( texture_flags != other.texture_flags )
        {
                return texture_flags < other.texture_flags;
        }
        if ( textures.size() != other.textures.size() )
        {
                return textures.size() < other.textures.size();
        }
        for ( size_t i = 0; i < textures.size(); ++i )
        {
                const textureinfo_t &tex = textures[i];
                const textureinfo_t &other_tex = other.textures[i];
                if ( tex.texcoord_name != other_tex.texcoord_name )
                {
                        return tex.texcoord_name < other_tex.texcoord_name;
                }
                if ( tex.type != other_tex.type )
                {
                        return tex.type < other_tex.type;
                }
                if ( tex.mode != other_tex.mode )
                {
                        return tex.mode < other_tex.mode;
                }
                if ( tex.gen_mode != other_tex.gen_mode )
                {
                        return tex.gen_mode < other_tex.gen_mode;
                }
                if ( tex.flags != other_tex.flags )
                {
                        return tex.flags < other_tex.flags;
                }
                if ( tex.combine_rgb != other_tex.combine_rgb )
                {
                        return tex.combine_rgb < other_tex.combine_rgb;
                }
                if ( tex.combine_alpha != other_tex.combine_alpha )
                {
                        return tex.combine_alpha < other_tex.combine_alpha;
                }
        }

        if ( fog_mode != other.fog_mode )
        {
                return fog_mode < other.fog_mode;
        }
        if ( outputs != other.outputs )
        {
                return outputs < other.outputs;
        }
        if ( calc_primary_alpha != other.calc_primary_alpha )
        {
                return calc_primary_alpha < other.calc_primary_alpha;
        }
        if ( disable_alpha_write != other.disable_alpha_write )
        {
                return disable_alpha_write < other.disable_alpha_write;
        }
        if ( alpha_test_mode != other.alpha_test_mode )
        {
                return alpha_test_mode < other.alpha_test_mode;
        }
        if ( alpha_test_ref != other.alpha_test_ref )
        {
                return alpha_test_ref < other.alpha_test_ref;
        }
        if ( num_clip_planes != other.num_clip_planes )
        {
                return num_clip_planes < other.num_clip_planes;
        }
        return light_ramp < other.light_ramp;
}

bool BSPShaderGenerator::shaderinfo_t::operator == ( const shaderinfo_t &other ) const
{
        if ( anim_spec != other.anim_spec )
        {
                return false;
        }
        if ( color_type != other.color_type )
        {
                return false;
        }
        if ( material_flags != other.material_flags )
        {
                return false;
        }
        if ( texture_flags != other.texture_flags )
        {
                return false;
        }
        if ( textures.size() != other.textures.size() )
        {
                return false;
        }
        if ( shade_model != other.shade_model )
        {
                return false;
        }
        for ( size_t i = 0; i < textures.size(); ++i )
        {
                const textureinfo_t &tex = textures[i];
                const textureinfo_t &other_tex = other.textures[i];
                if ( tex.texcoord_name != other_tex.texcoord_name ||
                     tex.type != other_tex.type ||
                     tex.mode != other_tex.mode ||
                     tex.gen_mode != other_tex.gen_mode ||
                     tex.flags != other_tex.flags ||
                     tex.combine_rgb != other_tex.combine_rgb ||
                     tex.combine_alpha != other_tex.combine_alpha )
                {
                        return false;
                }
        }
        return fog_mode == other.fog_mode
                && outputs == other.outputs
                && calc_primary_alpha == other.calc_primary_alpha
                && disable_alpha_write == other.disable_alpha_write
                && alpha_test_mode == other.alpha_test_mode
                && alpha_test_ref == other.alpha_test_ref
                && num_clip_planes == other.num_clip_planes
                && light_ramp == other.light_ramp;
}

/**
 * Returns 1D, 2D, 3D or CUBE, depending on the given texture type.
 */
const char *
texture_type_as_string( Texture::TextureType ttype )
{
        switch ( ttype )
        {
        case Texture::TT_1d_texture:
                return "1D";
                break;
        case Texture::TT_2d_texture:
                return "2D";
                break;
        case Texture::TT_3d_texture:
                return "3D";
                break;
        case Texture::TT_cube_map:
                return "Cube";
                break;
        case Texture::TT_2d_texture_array:
                return "2DArray";
                break;
        default:
                cout << "Unsupported texture type!\n";
                return "2D";
        }
}

/**
 * This 'synthesizes' a combine source into a string.
 */
string
combine_source_as_string( const BSPShaderGenerator::shaderinfo_t::textureinfo_t &info, short num, bool alpha, short texindex )
{
        TextureStage::CombineSource c_src;
        TextureStage::CombineOperand c_op;
        if ( !alpha )
        {
                c_src = UNPACK_COMBINE_SRC( info.combine_rgb, num );
                c_op = UNPACK_COMBINE_OP( info.combine_rgb, num );
        }
        else
        {
                c_src = UNPACK_COMBINE_SRC( info.combine_alpha, num );
                c_op = UNPACK_COMBINE_OP( info.combine_alpha, num );
        }
        std::ostringstream csource;
        if ( c_op == TextureStage::CO_one_minus_src_color ||
             c_op == TextureStage::CO_one_minus_src_alpha )
        {
                csource << "clamp(1.0f - ";
        }
        switch ( c_src )
        {
        case TextureStage::CS_undefined:
        case TextureStage::CS_texture:
                csource << "tex" << texindex;
                break;
        case TextureStage::CS_constant:
                csource << "texcolor_" << texindex;
                break;
        case TextureStage::CS_primary_color:
                csource << "primary_color";
                break;
        case TextureStage::CS_previous:
                csource << "result";
                break;
        case TextureStage::CS_constant_color_scale:
                csource << "attr_colorscale";
                break;
        case TextureStage::CS_last_saved_result:
                csource << "last_saved_result";
                break;
        }
        if ( c_op == TextureStage::CO_one_minus_src_color ||
             c_op == TextureStage::CO_one_minus_src_alpha )
        {
                csource << ", 0, 1)";
        }
        if ( c_op == TextureStage::CO_src_color || c_op == TextureStage::CO_one_minus_src_color )
        {
                csource << ".rgb";
        }
        else
        {
                csource << ".a";
                if ( !alpha )
                {
     // Dunno if it's legal in the FPP at all, but let's just allow it.
                        return "vec3(" + csource.str() + ")";
                }
        }
        return csource.str();
}

/**
 * This 'synthesizes' a combine mode into a string.
 */
string
combine_mode_as_string( const BSPShaderGenerator::shaderinfo_t::textureinfo_t &info, TextureStage::CombineMode c_mode, bool alpha, short texindex )
{
        std::ostringstream text;
        switch ( c_mode )
        {
        case TextureStage::CM_modulate:
                text << combine_source_as_string( info, 0, alpha, texindex );
                text << " * ";
                text << combine_source_as_string( info, 1, alpha, texindex );
                break;
        case TextureStage::CM_add:
                text << combine_source_as_string( info, 0, alpha, texindex );
                text << " + ";
                text << combine_source_as_string( info, 1, alpha, texindex );
                break;
        case TextureStage::CM_add_signed:
                text << combine_source_as_string( info, 0, alpha, texindex );
                text << " + ";
                text << combine_source_as_string( info, 1, alpha, texindex );
                if ( alpha )
                {
                        text << " - 0.5";
                }
                else
                {
                        text << " - vec3(0.5, 0.5, 0.5)";
                }
                break;
        case TextureStage::CM_interpolate:
                text << "lerp(";
                text << combine_source_as_string( info, 1, alpha, texindex );
                text << ", ";
                text << combine_source_as_string( info, 0, alpha, texindex );
                text << ", ";
                text << combine_source_as_string( info, 2, alpha, texindex );
                text << ")";
                break;
        case TextureStage::CM_subtract:
                text << combine_source_as_string( info, 0, alpha, texindex );
                text << " - ";
                text << combine_source_as_string( info, 1, alpha, texindex );
                break;
        case TextureStage::CM_dot3_rgb:
        case TextureStage::CM_dot3_rgba:
                text << "4 * dot(";
                text << combine_source_as_string( info, 0, alpha, texindex );
                text << " - vec3(0.5), ";
                text << combine_source_as_string( info, 1, alpha, texindex );
                text << " - vec3(0.5))";
                break;
        case TextureStage::CM_replace:
        default: // Not sure if this is correct as default value.
                text << combine_source_as_string( info, 0, alpha, texindex );
                break;
        }
        return text.str();
}

BSPShaderGenerator::BSPShaderGenerator()
{
}

CPT( ShaderAttrib ) BSPShaderGenerator::generate_shader( CPT( RenderState ) net_state )
{
	shaderinfo_t key;
        analyze_renderstate( key, net_state );
        auto si = _shader_cache.find( key );
        if ( si != _shader_cache.end() )
        {
                // We've already generated a shader for this state.
                return si->second;
        }

        //cout << "Generating BSP dynamic shader for state:\n";
        //net_state->write( cout, 2 );
        //cout << "\n";

        // Generate the shader source code.

        //============================
        // vertex shader
        //============================

        stringstream vshader;

        vshader << "#version 430\n";
        vshader << "/* Generated BSP dynamic object shader for render state:\n";
        net_state->write( vshader, 2 );
        vshader << "*/\n";

        int map_index_glow = -1;
        int map_index_gloss = -1;
        bool need_world_position = ( key.num_clip_planes > 0 );
        bool need_world_normal = true; // always required by ambient cube
        bool need_eye_position = true;
        bool need_eye_normal = true;

        bool have_specular = false;
        if ( key.material_flags & Material::F_specular ||
             key.texture_flags & shaderinfo_t::TEXTUREFLAGS_GLOSSMAP )
        {
                have_specular = true;
        }

        bool have_rim = false;
        if ( key.material_flags & Material::F_rim_color &&
             key.material_flags & Material::F_rim_width )
        {
                have_rim = true;
                need_eye_normal = true;
                need_eye_position = true;
        }

        bool have_lightwarp = false;
        if ( key.material_flags & Material::F_lightwarp_texture )
        {
                have_lightwarp = true;
        }

        bool need_color = false;
        if ( key.color_type != ColorAttrib::T_off )
        {
                need_color = true;
        }

        vector_string texcoords;
        string tangent_input;
        string binormal_input;

        for ( size_t i = 0; i < key.textures.size(); i++ )
        {
                const shaderinfo_t::textureinfo_t &texinfo = key.textures[i];

                switch ( texinfo.gen_mode )
                {
                case TexGenAttrib::M_world_position:
                        need_world_position = true;
                        break;
                case TexGenAttrib::M_world_normal:
                        need_world_normal = true;
                        break;
                case TexGenAttrib::M_eye_position:
                        need_eye_position = true;
                        break;
                case TexGenAttrib::M_eye_normal:
                        need_eye_normal = true;
                        break;
                }

                if ( texinfo.texcoord_name != nullptr )
                {
                        string tcname = texinfo.texcoord_name->join( "_" );
                        if ( std::find( texcoords.begin(), texcoords.end(), tcname ) == texcoords.end() )
                        {
                                vshader << "in vec4 " << tcname << ";\n";
                                vshader << "out vec4 l_" << tcname << ";\n";
                                texcoords.push_back( tcname );
                        }
                }

                if ( tangent_input.empty() &&
                     ( texinfo.flags & ( shaderinfo_t::TEXTUREFLAGS_NORMALMAP | shaderinfo_t::TEXTUREFLAGS_HEIGHTMAP | shaderinfo_t::TEXTUREFLAGS_SPHEREMAP ) ) )
                {
                        PT( InternalName ) tangent_name = InternalName::get_tangent();
                        PT( InternalName ) binormal_name = InternalName::get_binormal();

                        if ( texinfo.texcoord_name != nullptr &&
                             texinfo.texcoord_name != InternalName::get_texcoord() )
                        {
                                tangent_name = tangent_name->append( texinfo.texcoord_name->get_basename() );
                                binormal_name = binormal_name->append( texinfo.texcoord_name->get_basename() );
                        }

                        tangent_input = tangent_name->join( "_" );
                        binormal_input = binormal_name->join( "_" );

                        vshader << "in vec4 " << tangent_input << ";\n";
                        vshader << "in vec4 " << binormal_input << ";\n";
                }

                if ( texinfo.flags & shaderinfo_t::TEXTUREFLAGS_GLOWMAP )
                {
                        map_index_glow = i;
                }
                if ( texinfo.flags & shaderinfo_t::TEXTUREFLAGS_GLOSSMAP )
                {
                        map_index_gloss = i;
                }
        }

        if ( key.texture_flags & shaderinfo_t::TEXTUREFLAGS_NORMALMAP )
        {
                vshader << "out vec3 l_tangent;\n";
                vshader << "out vec3 l_binormal;\n";
        }
        if ( need_color && key.color_type == ColorAttrib::T_vertex )
        {
                vshader << "in vec4 p3d_Color;\n";
                vshader << "out vec4 l_color;\n";
        }
        if ( need_world_position || need_world_normal )
        {
                vshader << "uniform mat4 p3d_ModelMatrix;\n";
        }
        if ( need_world_position )
        {
                vshader << "out vec4 l_world_position;\n";
        }
        if ( need_world_normal )
        {
                vshader << "out vec4 l_world_normal;\n";
        }
        if ( need_eye_position )
        {
                vshader << "uniform mat4 p3d_ModelViewMatrix;\n";
                vshader << "out vec4 l_eye_position;\n";
        }
        else if ( key.texture_flags & shaderinfo_t::TEXTUREFLAGS_NORMALMAP )
        {
                vshader << "uniform mat4 p3d_ModelViewMatrix;\n";
        }
        if ( need_eye_normal )
        {
                vshader << "uniform mat4 tpose_view_to_model;\n";
                vshader << "out vec4 l_eye_normal_orig;\n";
        }
        if ( key.texture_flags & shaderinfo_t::TEXTUREFLAGS_HEIGHTMAP || need_world_normal || need_eye_normal )
        {
                vshader << "in vec3 p3d_Normal;\n";
        }
        if ( key.texture_flags & ( shaderinfo_t::TEXTUREFLAGS_HEIGHTMAP | shaderinfo_t::TEXTUREFLAGS_SPHEREMAP ) )
        {
                vshader << "uniform vec4 mspos_view;\n";
                vshader << "out vec3 l_eyevec;\n";
        }
        if ( key.fog_mode != 0 )
        {
                vshader << "out vec4 l_hpos;\n";
        }

        vshader << "uniform vec3 ambient_cube[6];\n";
        vshader << "out vec4 l_ambient_term;\n";

        vshader << "in vec4 p3d_Vertex;\n";
        vshader << "out vec4 l_position;\n";
        vshader << "uniform mat4 p3d_ModelViewProjectionMatrix;\n";

        //===================================
        // Ambient cube sampler function
        vshader << "vec3 ambient_light(vec3 world_normal) {\n";
        vshader << "\t vec3 n_sqr = world_normal * world_normal;\n";
        vshader << "\t int negative = 0;\n";
        vshader << "\t if (n_sqr.x < 0.0 && n_sqr.y < 0.0 && n_sqr.z < 0.0) {\n";
        vshader << "\t\t negative = 1;}\n";
        vshader << "\t vec3 linear = n_sqr.x * ambient_cube[negative] + \n";
        vshader << "\t\t n_sqr.y * ambient_cube[negative + 2] + \n";
        vshader << "\t\t n_sqr.z * ambient_cube[negative + 4];\n";
        vshader << "\t return linear;\n";
        vshader << "}\n";
        //===================================

        vshader << "void main() {\n";

        vshader << "\t gl_Position = p3d_ModelViewProjectionMatrix * p3d_Vertex;\n";
        if ( key.fog_mode != 0 )
        {
                vshader << "\t l_hpos = l_position;\n";
        }
        if ( need_world_position )
        {
                vshader << "\t l_world_position = p3d_ModelMatrix * p3d_Vertex;\n";
        }
        if ( need_world_normal )
        {
                vshader << "\t l_world_normal = p3d_ModelMatrix * vec4(p3d_Normal, 0);\n";
        }
        if ( need_eye_position )
        {
                vshader << "\t l_eye_position = p3d_ModelViewMatrix * p3d_Vertex;\n";
        }
        if ( need_eye_normal )
        {
                vshader << "\t l_eye_normal_orig.xyz = normalize(mat3(tpose_view_to_model) * p3d_Normal);\n";
                vshader << "\t l_eye_normal_orig.w = 0;\n";
        }

        for ( size_t i = 0; i < texcoords.size(); i++ )
        {
                // Pass through all texcoord inputs as-is.
                string tcname = texcoords[i];
                vshader << "\t l_" << tcname << " = " << tcname << ";\n";
        }
        
        if ( need_color && key.color_type == ColorAttrib::T_vertex )
        {
                vshader << "\t l_color = p3d_Color;\n";
        }

        if ( key.texture_flags & shaderinfo_t::TEXTUREFLAGS_NORMALMAP )
        {
                vshader << "\t l_tangent = normalize(mat3(p3d_ModelViewMatrix) * " << tangent_input << ".xyz);\n";
                vshader << "\t l_binormal = normalize(mat3(p3d_ModelViewMatrix) * -" << binormal_input << ".xyz);\n";
        }

        if ( key.texture_flags & ( shaderinfo_t::TEXTUREFLAGS_HEIGHTMAP | shaderinfo_t::TEXTUREFLAGS_SPHEREMAP ) )
        {
                vshader << "\t vec3 eyedir = mspos_view.xyz - p3d_Vertex.xyz;\n";
                vshader << "\t l_eyevec.x = dot(" << tangent_input << ".xyz, eyedir);\n";
                vshader << "\t l_eyevec.y = dot(" << binormal_input << ".xyz, eyedir);\n";
                vshader << "\t l_eyevec.z = dot(p3d_Normal, eyedir);\n";
                vshader << "\t l_eyevec = normalize(l_eyevec);\n";
        }

        vshader << "\t vec3 ambient = ambient_light(l_world_normal.xyz);\n";
        vshader << "\t l_ambient_term = vec4(ambient, 1.0);\n";

        vshader << "}\n\n";

        //============================
        // pixel shader
        //============================

        stringstream pshader;
        pshader << "#version 430\n";

        if ( key.fog_mode != 0 )
        {
                pshader << "in vec4 l_hpos;\n";
                pshader << "uniform vec4 attr_fog;\n";
                pshader << "uniform vec4 attr_fogcolor;\n";
        }
        if ( need_world_position )
        {
                pshader << "in vec4 l_world_position;\n";
        }
        if ( need_world_normal )
        {
                pshader << "in vec4 l_world_normal;\n";
        }
        if ( need_eye_position )
        {
                pshader << "in vec4 l_eye_position;\n";
        }
        if ( need_eye_normal )
        {
                pshader << "in vec4 l_eye_normal_orig;\n";
        }
        for ( size_t i = 0; i < texcoords.size(); i++ )
        {
                pshader << "in vec4 l_" << texcoords[i] << ";\n";
        }
        for ( size_t i = 0; i < key.textures.size(); i++ )
        {
                const shaderinfo_t::textureinfo_t &texinfo = key.textures[i];
                if ( texinfo.mode == TextureStage::M_modulate && texinfo.flags == 0 )
                {
                        // Skip this stage.
                        continue;
                }

                pshader << "uniform sampler" << texture_type_as_string( texinfo.type ) << " p3d_Texture" << i << ";\n";

                if ( texinfo.flags & shaderinfo_t::TEXTUREFLAGS_HAS_SCALE )
                {
                        pshader << "uniform vec3 texscale_" << i << ";\n";
                }
                else if ( texinfo.flags & shaderinfo_t::TEXTUREFLAGS_HAS_MAT )
                {
                        pshader << "uniform mat4 p3d_TextureMatrix[" << i << "];\n";
                }

                if ( texinfo.flags & shaderinfo_t::TEXTUREFLAGS_USES_COLOR )
                {
                        pshader << "uniform vec4 texcolor_" << i << ";\n";
                }
        }

        if ( key.texture_flags & shaderinfo_t::TEXTUREFLAGS_NORMALMAP )
        {
                pshader << "in vec3 l_tangent;\n";
                pshader << "in vec3 l_binormal;\n";
        }

        // Does the shader need material properties as input?
        if ( key.material_flags != 0 )
        {
                pshader << "uniform struct {\n";
                if ( key.material_flags & Material::F_ambient )
                {
                        pshader << "\t vec4 ambient;\n";
                }
                if ( key.material_flags & Material::F_diffuse )
                {
                        pshader << "\t vec4 diffuse;\n";
                }
                if ( key.material_flags & Material::F_emission )
                {
                        pshader << "\t vec4 emission;\n";
                }
                if ( key.material_flags & Material::F_specular )
                {
                        pshader << "\t vec3 specular;\n";
                        pshader << "\t float shininess;\n";
                }

                if ( key.material_flags & Material::F_rim_color && key.material_flags & Material::F_rim_width )
                {
                        pshader << "\t vec4 rimColor;\n";
                        pshader << "\t float rimWidth;\n";
                }

                if ( have_lightwarp )
                {
                        pshader << "\t sampler2D lightwarp;\n";
                }

                pshader << "} p3d_Material;\n";
        }

        if ( key.texture_flags & (shaderinfo_t::TEXTUREFLAGS_HEIGHTMAP | shaderinfo_t::TEXTUREFLAGS_SPHEREMAP) )
        {
                pshader << "in vec3 l_eyevec;\n";
        }

        if ( key.texture_flags & shaderinfo_t::TEXTUREFLAGS_SPHEREMAP )
        {
                pshader << "uniform mat4 p3d_ViewMatrixInverse;\n";
        }

        if ( key.outputs & ( AuxBitplaneAttrib::ABO_aux_normal | AuxBitplaneAttrib::ABO_aux_glow ) )
        {
                pshader << "out vec4 o_aux;\n";
        }        

        if ( need_color )
        {
                if ( key.color_type == ColorAttrib::T_vertex )
                {
                        pshader << "in vec4 l_color;\n";
                }
                else if ( key.color_type == ColorAttrib::T_flat )
                {
                        pshader << "uniform vec4 p3d_Color;\n";
                }
        }

        if ( key.num_clip_planes > 0 )
        {
                pshader << "uniform vec4 p3d_ClipPlane[" << key.num_clip_planes << "];\n";
        }

        pshader << "uniform vec4 p3d_ColorScale;\n";

        pshader << "uniform mat4 p3d_ViewMatrix;\n";
        pshader << "in vec4 l_ambient_term;\n";
        pshader << "uniform int light_count[1];\n";
        pshader << "uniform int light_type[" << MAXLIGHTS << "];\n";
        pshader << "uniform vec3 light_pos[" << MAXLIGHTS << "];\n";
        pshader << "uniform vec4 light_direction[" << MAXLIGHTS << "];\n";
        pshader << "uniform vec4 light_atten[" << MAXLIGHTS << "];\n";
        pshader << "uniform vec3 light_color[" << MAXLIGHTS << "];\n";

        pshader << "const int LIGHTTYPE_SUN = 0;\n";
        pshader << "const int LIGHTTYPE_POINT = 1;\n";
        pshader << "const int LIGHTTYPE_SPOT = 2;\n";

        pshader << "out vec4 o_color;\n";

        if ( key.shade_model == Material::SM_half_lambert )
        {
                pshader << "float half_lambert(float dp)\n";
                pshader << "{\n";
                pshader << "\t float hl = dp * 0.5;\n";
                pshader << "\t hl += 0.5;\n";
                pshader << "\t hl *= hl;\n";
                pshader << "\t return hl;\n";
                pshader << "}\n";
        }

        pshader << "void main() {\n";

        // Clipping first!
        for ( int i = 0; i < key.num_clip_planes; i++ )
        {
                pshader << "\t if (l_world_position.x * p3d_ClipPlane[" << i << "].x + l_world_position.y ";
                pshader << "* p3d_ClipPlane[" << i << "].y + l_world_position.z * p3d_ClipPlane[" << i << "].z ";
                pshader << "+ p3d_ClipPlane[" << i << "].w <= 0) { discard; };\n";
        }

        pshader << "\t vec4 l_eye_normal = l_eye_normal_orig;\n";

        pshader << "\t vec4 result;\n";
        if ( key.outputs & ( AuxBitplaneAttrib::ABO_aux_normal | AuxBitplaneAttrib::ABO_aux_glow ) )
        {
                pshader << "\t o_aux = vec4(0, 0, 0, 0);\n";
        }

        // Now generate any texture coordinates according to TexGenAttrib.  If it
        // has a TexMatrixAttrib, also transform them.
        for ( size_t i = 0; i < key.textures.size(); ++i )
        {
                const shaderinfo_t::textureinfo_t &tex = key.textures[i];
                if ( tex.mode == TextureStage::M_modulate && tex.flags == 0 )
                {
                        // Skip this stage.
                        continue;
                }
                switch ( tex.gen_mode )
                {
                case TexGenAttrib::M_off:
                  // Cg seems to be able to optimize this temporary away when appropriate.
                        pshader << "\t vec4 texcoord" << i << " = l_" << tex.texcoord_name->join( "_" ) << ";\n";
                        break;
                case TexGenAttrib::M_world_position:
                        pshader << "\t vec4 texcoord" << i << " = l_world_position;\n";
                        break;
                case TexGenAttrib::M_world_normal:
                        pshader << "\t vec4 texcoord" << i << " = l_world_normal;\n";
                        break;
                case TexGenAttrib::M_eye_position:
                        pshader << "\t vec4 texcoord" << i << " = l_eye_position;\n";
                        break;
                case TexGenAttrib::M_eye_normal:
                        pshader << "\t vec4 texcoord" << i << " = l_eye_normal;\n";
                        pshader << "\t texcoord" << i << ".w = 1.0f;\n";
                        break;
                case TexGenAttrib::M_eye_sphere_map:
                        pshader << "\t vec3 r" << i << " = reflect(l_eyevec.xyz, l_eye_normal.xyz);\n";
                        pshader << "\t r" << i << " = vec3(p3d_ViewMatrixInverse * vec4(r" << i << ", 0.0));\n";
                        pshader << "\t float m" << i << " = 2.0 * sqrt(pow(r" << i << ".x, 2.0) + pow(r" << i << ".y, 2.0) + pow(r" << i << ".z + 1.0, 2.0));\n";
                        pshader << "\t vec4 texcoord" << i << " = vec4(r" << i << ".x / m" << i << " + 0.5, r" << i << ".y / m" << i << " + 0.5, 0, 0);\n";
                        break;
                default:
                        pshader << "\t vec4 texcoord" << i << " = vec4(0, 0, 0, 0);\n";
                        cout
                                << "Unsupported TexGenAttrib mode: " << tex.gen_mode << "\n";
                }
                if ( tex.flags & shaderinfo_t::TEXTUREFLAGS_HAS_SCALE )
                {
                        pshader << "\t texcoord" << i << ".xyz *= texscale_" << i << ";\n";
                }
                else if ( tex.flags & shaderinfo_t::TEXTUREFLAGS_HAS_MAT )
                {
                        pshader << "\t texcoord" << i << " = p3d_TextureMatrix[" << i << "] * texcoord" << i << ";\n";
                        pshader << "\t texcoord" << i << ".xyz /= texcoord" << i << ".w;\n";
                }
        }
        pshader << "\t // Fetch all textures.\n";
        for ( size_t i = 0; i < key.textures.size(); ++i )
        {
                const shaderinfo_t::textureinfo_t &tex = key.textures[i];
                if ( ( tex.flags & shaderinfo_t::TEXTUREFLAGS_HEIGHTMAP ) == 0 )
                {
                        continue;
                }

                pshader << "\t vec4 tex" << i << " = texture" << texture_type_as_string( tex.type );
                pshader << "(p3d_Texture" << i << ", texcoord" << i << ".";
                switch ( tex.type )
                {
                case Texture::TT_cube_map:
                case Texture::TT_3d_texture:
                case Texture::TT_2d_texture_array:
                        pshader << "xyz";
                        break;
                case Texture::TT_2d_texture:
                        pshader << "xy";
                        break;
                case Texture::TT_1d_texture:
                        pshader << "x";
                        break;
                default:
                        break;
                }
                pshader << ");\n\t vec3 parallax_offset = l_eyevec.xyz * (tex" << i;
                if ( tex.mode == TextureStage::M_normal_height ||
                        ( tex.flags & shaderinfo_t::TEXTUREFLAGS_HAS_ALPHA ) != 0 )
                {
                        pshader << ".aaa";
                }
                else
                {
                        pshader << ".rgb";
                }
                pshader << " * 2.0 - 1.0) * " << parallax_mapping_scale << ";\n";
                // Additional samples
                for ( int j = 0; j < parallax_mapping_samples - 1; ++j )
                {
                        pshader << "\t parallax_offset = l_eyevec.xyz * (parallax_offset + (tex" << i;
                        if ( tex.mode == TextureStage::M_normal_height ||
                                ( tex.flags & shaderinfo_t::TEXTUREFLAGS_HAS_ALPHA ) != 0 )
                        {
                                pshader << ".aaa";
                        }
                        else
                        {
                                pshader << ".rgb";
                        }
                        pshader << " * 2.0 - 1.0)) * " << 0.5 * parallax_mapping_scale << ";\n";
                }
        }
        for ( size_t i = 0; i < key.textures.size(); ++i )
        {
                shaderinfo_t::textureinfo_t &tex = key.textures[i];
                if ( tex.mode == TextureStage::M_modulate && tex.flags == 0 )
                {
                        // Skip this stage.
                        continue;
                }
                if ( ( tex.flags & shaderinfo_t::TEXTUREFLAGS_HEIGHTMAP ) == 0 )
                {
                        // Parallax mapping pushes the texture coordinates of the other textures
                        // away from the camera.
                        if ( key.texture_flags & shaderinfo_t::TEXTUREFLAGS_HEIGHTMAP )
                        {
                                pshader << "\t texcoord" << i << ".xyz -= parallax_offset;\n";
                        }
                        pshader << "\t vec4 tex" << i << " = texture" << texture_type_as_string( tex.type );
                        pshader << "(p3d_Texture" << i << ", texcoord" << i << ".";
                        switch ( tex.type )
                        {
                        case Texture::TT_cube_map:
                        case Texture::TT_3d_texture:
                        case Texture::TT_2d_texture_array:
                                pshader << "xyz";
                                break;
                        case Texture::TT_2d_texture:
                                pshader << "xy";
                                break;
                        case Texture::TT_1d_texture:
                                pshader << "x";
                                break;
                        default:
                                break;
                        }
                        pshader << ");\n";
                }
        }

        if ( key.texture_flags & shaderinfo_t::TEXTUREFLAGS_NORMALMAP )
        {
                pshader << "\t // Translate tangent-space normal in map to view-space.\n";

                // Use Reoriented Normal Mapping to blend additional normal maps.
                bool is_first = true;
                for ( size_t i = 0; i < key.textures.size(); ++i )
                {
                        const shaderinfo_t::textureinfo_t &tex = key.textures[i];
                        if ( tex.flags & shaderinfo_t::TEXTUREFLAGS_NORMALMAP )
                        {
                                if ( is_first )
                                {
                                        pshader << "\t vec3 tsnormal = (tex" << i << ".xyz * 2) - 1;\n";
                                        is_first = false;
                                        continue;
                                }
                                pshader << "\t tsnormal.z += 1;\n";
                                pshader << "\t vec3 tmp" << i << " = tex" << i << ".xyz * vec3(-2, -2, 2) + vec3(1, 1, -1);\n";
                                pshader << "\t tsnormal = normalize(tsnormal * dot(tsnormal, tmp" << i << ") - tmp" << i << " * tsnormal.z);\n";
                        }
                }
                pshader << "\t l_eye_normal.xyz *= tsnormal.z;\n";
                pshader << "\t l_eye_normal.xyz += l_tangent * tsnormal.x;\n";
                pshader << "\t l_eye_normal.xyz += l_binormal * tsnormal.y;\n";
        }
        if ( need_eye_normal )
        {
                pshader << "\t // Correct the surface normal for interpolation effects\n";
                pshader << "\t l_eye_normal.xyz = normalize(l_eye_normal.xyz);\n";
        }
        if ( key.outputs & AuxBitplaneAttrib::ABO_aux_normal )
        {
                pshader << "\t // Output the camera-space surface normal\n";
                pshader << "\t o_aux.rgb = (l_eye_normal.xyz*0.5) + vec3(0.5,0.5,0.5);\n";
        }

        pshader << "\t // Begin view-space light calculations\n";
        pshader << "\t float ldist,lattenv,langle,lshad,lintensity;\n";
        pshader << "\t vec4 lcolor,lpoint,latten,ldir,leye;\n";
        pshader << "\t vec3 lvec,lhalf,lspec;\n";
        if ( have_rim )
        {
                pshader << "\t vec4 tot_rim = vec4(0, 0, 0, 0);\n";
        }
        pshader << "\t vec4 tot_diffuse = vec4(0, 0, 0, 0);\n";
        if ( have_specular )
        {
                pshader << "\t vec3 tot_specular = vec3(0, 0, 0);\n";
                if ( key.material_flags & Material::F_specular )
                {
                        pshader << "\t float shininess = p3d_Material.shininess;\n";
                }
                else
                {
                        pshader << "\t float shininess = 50; // no shininess specified, using default\n";
                }
        }
        // Start with ambient cube lighting
        pshader << "\t tot_diffuse += l_ambient_term;\n";

        if ( have_rim )
        {
                // Apply rim term
                pshader << "\t vec3 rim_eye_pos = normalize(-l_eye_position.xyz);\n";
                pshader << "\t float rim_intensity = p3d_Material.rimWidth - max(dot(rim_eye_pos, l_eye_normal.xyz), 0.0);\n";
                pshader << "\t rim_intensity = max(0.0, rim_intensity);\n";
                pshader << "\t tot_rim += vec4(rim_intensity * p3d_Material.rimColor);\n";
        }

        // Now factor in local light sources
        pshader << "\t for (int i = 0; i < light_count[0]; i++) {\n";

        pshader << "\t         lcolor = vec4(light_color[i], 1.0);\n";
        pshader << "\t         latten = light_atten[i];\n";

        // point lights
        pshader << "\t         if (light_type[i] == LIGHTTYPE_POINT) {\n";
        pshader << "\t             lpoint = p3d_ViewMatrix * vec4(light_pos[i], 1);\n";
        pshader << "\t             lvec = lpoint.xyz - l_eye_position.xyz;\n";
        pshader << "\t             ldist = length(lvec);\n";
        pshader << "\t             lvec = normalize(lvec);\n";
        pshader << "\t             lintensity = clamp(dot(l_eye_normal.xyz, lvec), 0, 1);\n";
        if ( key.shade_model == Material::SM_half_lambert )
        {
                pshader << "\t     lintensity = half_lambert(lintensity);\n";
        }
        if ( have_lightwarp )
        {
                pshader << "\t     lintensity = texture2D(p3d_Material.lightwarp, vec2(lintensity * 0.5 + 0.5, 0.5));\n";
        }
        pshader << "\t             lattenv = ldist * latten.x;\n";
        pshader << "\t             float ratio = lintensity / lattenv;\n";
        pshader << "\t             lcolor *= ratio;\n";
        pshader << "\t             tot_diffuse += lcolor;\n";
        if ( have_specular )
        {
                if ( key.material_flags & Material::F_local )
                {
                        pshader << "\t lhalf = normalize(lvec - normalize(l_eye_position.xyz));\n";
                }
                else
                {
                        pshader << "\t lhalf = normalize(lvec - vec3(0, 1, 0));\n";
                }
                pshader << "\t     lspec = p3d_Material.specular;\n";
                pshader << "\t     lspec *= lattenv;\n";
                pshader << "\t     lspec *= pow(clamp(dot(l_eye_normal.xyz, lhalf), 0, 1), shininess);\n";
                pshader << "\t     tot_specular += lspec;\n";
        }
        pshader << "\t         }\n";

        // sun/directional light
        pshader << "\t         else if(light_type[i] == LIGHTTYPE_SUN) {\n";
        pshader << "\t             lvec = normalize((p3d_ViewMatrix * light_direction[i]).xyz);\n";
        pshader << "\t             lintensity = clamp(dot(l_eye_normal.xyz, -lvec), 0, 1);\n";
        if ( key.shade_model == Material::SM_half_lambert )
        {
                pshader << "\t     lintensity = half_lambert(lintensity);\n";
        }
        if ( have_lightwarp )
        {
                pshader << "\t     lcolor *= texture2D(p3d_Material.lightwarp, vec2(lintensity * 0.5 + 0.5, 0.5));\n";
        }
        else
        {
                pshader << "\t     lcolor *= lintensity;\n";
        }
        pshader << "\t             tot_diffuse += lcolor;\n";
        if ( have_specular )
        {
                if ( key.material_flags & Material::F_local )
                {
                        pshader << "\t lhalf = normalize(lvec - normalize(l_eye_position.xyz));\n";
                }
                else
                {
                        pshader << "\t lhalf = normalize(lvec - vec3(0, 1, 0));\n";
                }
                pshader << "\t     lspec = p3d_Material.specular;\n";
                pshader << "\t     lspec *= pow(clamp(dot(l_eye_normal.xyz, lhalf), 0, 1), shininess);\n";
                pshader << "\t     tot_specular += lspec;\n";
        }
        pshader << "\t         }\n";

        // spotlights
        pshader << "\t         else if (light_type[i] == LIGHTTYPE_SPOT) {\n";
        pshader << "\t             lpoint = p3d_ViewMatrix * vec4(light_pos[i], 1);\n";
        pshader << "\t             ldir = normalize((p3d_ViewMatrix * light_direction[i]));\n";
        pshader << "\t             lvec = lpoint.xyz - l_eye_position.xyz;\n";
        pshader << "\t             ldist = length(lvec);\n";
        pshader << "\t             lvec = normalize(lvec);\n";
        pshader << "\t             lintensity = clamp(dot(l_eye_normal.xyz, lvec), 0, 1);\n";
        if ( key.shade_model == Material::SM_half_lambert )
        {
                pshader << "\t     lintensity = half_lambert(lintensity);\n";
        }
        if ( have_lightwarp )
        {
                pshader << "\t     lintensity = texture2D(p3d_Material.lightwarp, vec2(lintensity * 0.5 + 0.5, 0.5));\n";
        }
        pshader << "\t             float dot2 = clamp(dot(lvec, normalize(-ldir.xyz)), 0, 1);\n";
        pshader << "\t             if (dot2 <= latten.z) { /* outside light cone */ continue; }\n";
        pshader << "\t             float denominator = ldist * latten.x;\n";
        pshader << "\t             lattenv = lintensity * dot2 / denominator;\n";
        pshader << "\t             if (dot2 <= latten.y) { lattenv *= (dot2 - latten.z) / (latten.y - latten.z); }\n";
        pshader << "\t             lcolor *= lattenv;\n";
        pshader << "\t             tot_diffuse += lcolor;\n";
        if ( have_specular )
        {
                if ( key.material_flags & Material::F_local )
                {
                        pshader << "\t lhalf = normalize(lvec - normalize(l_eye_position.xyz));\n";
                }
                else
                {
                        pshader << "\t lhalf = normalize(lvec - vec3(0, 1, 0));\n";
                }
                pshader << "\t     lspec = p3d_Material.specular;\n";
                pshader << "\t     lspec *= lattenv;\n";
                pshader << "\t     lspec *= pow(clamp(dot(l_eye_normal.xyz, lhalf), 0, 1), shininess);\n";
                pshader << "\t     tot_specular += lspec;\n";
        }
        pshader << "\t         }\n";
        pshader << "\t }\n";

        pshader << "\t // Begin view-space light summation\n";
        if ( key.material_flags & Material::F_emission )
        {
                if ( key.texture_flags & shaderinfo_t::TEXTUREFLAGS_GLOWMAP )
                {
                        pshader << "\t result = p3d_Material.emission * clamp(2 * (tex" << map_index_glow << ".a - 0.5), 0, 1);\n";
                }
                else
                {
                        pshader << "\t result = p3d_Material.emission;\n";
                }
        }
        else
        {
                if ( key.texture_flags & shaderinfo_t::TEXTUREFLAGS_GLOWMAP )
                {
                        pshader << "\t result = vec4(clamp(2 * (tex" << map_index_glow << ".a - 0.5), 0, 1));\n";
                }
                else
                {
                        pshader << "\t result = vec4(0, 0, 0, 0);\n";
                }
        }
        if ( key.material_flags & Material::F_diffuse )
        {
                pshader << "\t result += tot_diffuse * p3d_Material.diffuse;\n";
        }
        else
        {
                pshader << "\t result += tot_diffuse;\n";
        }
        if ( have_rim )
        {
                pshader << "\t result += tot_rim;\n";
        }
        if ( key.color_type == ColorAttrib::T_vertex )
        {
                pshader << "\t result *= l_color;\n";
        }
        if ( key.color_type == ColorAttrib::T_flat )
        {
                pshader << "\t result *= p3d_Color;\n";
        }
        //if ( key.light_ramp == nullptr ||
        //     key.light_ramp->get_mode() == LightRampAttrib::LRT_default )
        //{
        //        pshader << "\t result = clamp(result, 0.0, 1.0);\n";
        //}
        pshader << "\t // End view-space light summations\n";

        // Combine in alpha, which bypasses lighting calculations.  Use of lerp
        // here is a workaround for a radeon driver bug.
        if ( key.calc_primary_alpha )
        {
                if ( key.color_type == ColorAttrib::T_vertex )
                {
                        pshader << "\t result.a = l_color.a;\n";
                }
                else if ( key.color_type == ColorAttrib::T_flat )
                {
                        pshader << "\t result.a = p3d_Color.a;\n";
                }
                else
                {
                        pshader << "\t result.a = 1;\n";
                }
        }

        // Apply the color scale.
        pshader << "\t result *= p3d_ColorScale;\n";

        // Store these if any stages will use it.
        if ( key.texture_flags & shaderinfo_t::TEXTUREFLAGS_USES_PRIMARY_COLOR )
        {
                pshader << "\t vec4 primary_color = result;\n";
        }
        if ( key.texture_flags & shaderinfo_t::TEXTUREFLAGS_USES_LAST_SAVED_RESULT )
        {
                pshader << "\t vec4 last_saved_result = result;\n";
        }

        // Now loop through the textures to compose our magic blending formulas.
        for ( size_t i = 0; i < key.textures.size(); ++i )
        {
                const shaderinfo_t::textureinfo_t &tex = key.textures[i];
                TextureStage::CombineMode combine_rgb, combine_alpha;

                switch ( tex.mode )
                {
                case TextureStage::M_modulate:
                        if ( ( tex.flags & shaderinfo_t::TEXTUREFLAGS_HAS_RGB ) != 0 &&
                                ( tex.flags & shaderinfo_t::TEXTUREFLAGS_HAS_ALPHA ) != 0 )
                        {
                                pshader << "\t result.rgba *= tex" << i << ".rgba;\n";
                        }
                        else if ( tex.flags & shaderinfo_t::TEXTUREFLAGS_HAS_ALPHA )
                        {
                                pshader << "\t result.a *= tex" << i << ".a;\n";
                        }
                        else if ( tex.flags & shaderinfo_t::TEXTUREFLAGS_HAS_RGB )
                        {
                                pshader << "\t result.rgb *= tex" << i << ".rgb;\n";
                        }
                        break;
                case TextureStage::M_modulate_glow:
                case TextureStage::M_modulate_gloss:
                  // in the case of glow or spec we currently see the specularity evenly
                  // across the surface even if transparency or masking is present not
                  // sure if this is desired behavior or not.  *MOST* would construct a
                  // spec map based off of what isisn't seen based on the masktransparency
                  // this may have to be left alone for now agartner
                        pshader << "\t result.rgb *= tex" << i << ";\n";
                        break;
                case TextureStage::M_decal:
                        pshader << "\t result.rgb = lerp(result, tex" << i << ", tex" << i << ".a).rgb;\n";
                        break;
                case TextureStage::M_blend:
                        pshader << "\t result.rgb = lerp(result.rgb, texcolor_" << i << ".rgb, tex" << i << ".rgb);\n";
                        if ( key.calc_primary_alpha )
                        {
                                pshader << "\t result.a *= tex" << i << ".a;\n";
                        }
                        break;
                case TextureStage::M_replace:
                        pshader << "\t result = tex" << i << ";\n";
                        break;
                case TextureStage::M_add:
                        pshader << "\t result.rgb += tex" << i << ".rgb;\n";
                        if ( key.calc_primary_alpha )
                        {
                                pshader << "\t result.a   *= tex" << i << ".a;\n";
                        }
                        break;
                case TextureStage::M_combine:
                        combine_rgb = ( TextureStage::CombineMode )( ( tex.flags & shaderinfo_t::TEXTUREFLAGS_COMBINE_RGB_MODE_MASK ) >> shaderinfo_t::TEXTUREFLAGS_COMBINE_RGB_MODE_SHIFT );
                        combine_alpha = ( TextureStage::CombineMode )( ( tex.flags & shaderinfo_t::TEXTUREFLAGS_COMBINE_ALPHA_MODE_MASK ) >> shaderinfo_t::TEXTUREFLAGS_COMBINE_ALPHA_MODE_SHIFT );
                        if ( combine_rgb == TextureStage::CM_dot3_rgba )
                        {
                                pshader << "\t result = ";
                                pshader << combine_mode_as_string( tex, combine_rgb, false, i );
                                pshader << ";\n";
                        }
                        else
                        {
                                pshader << "\t result.rgb = ";
                                pshader << combine_mode_as_string( tex, combine_rgb, false, i );
                                pshader << ";\n\t result.a = ";
                                pshader << combine_mode_as_string( tex, combine_alpha, true, i );
                                pshader << ";\n";
                        }
                        if ( tex.flags & shaderinfo_t::TEXTUREFLAGS_RGB_SCALE_2 )
                        {
                                pshader << "\t result.rgb *= 2;\n";
                        }
                        if ( tex.flags & shaderinfo_t::TEXTUREFLAGS_RGB_SCALE_4 )
                        {
                                pshader << "\t result.rgb *= 4;\n";
                        }
                        if ( tex.flags & shaderinfo_t::TEXTUREFLAGS_ALPHA_SCALE_2 )
                        {
                                pshader << "\t result.a *= 2;\n";
                        }
                        if ( tex.flags & shaderinfo_t::TEXTUREFLAGS_ALPHA_SCALE_4 )
                        {
                                pshader << "\t result.a *= 4;\n";
                        }
                        break;
                case TextureStage::M_blend_color_scale:
                        pshader << "\t result.rgb = lerp(result.rgb, texcolor_" << i << ".rgb * p3d_ColorScale.rgb, tex" << i << ".rgb);\n";
                        if ( key.calc_primary_alpha )
                        {
                                pshader << "\t result.a *= texcolor_" << i << ".a * p3d_ColorScale.a;\n";
                        }
                        break;
                default:
                        break;
                }
                if ( tex.flags & shaderinfo_t::TEXTUREFLAGS_SAVED_RESULT )
                {
                        pshader << "\t last_saved_result = result;\n";
                }
        }

        if ( key.alpha_test_mode != RenderAttrib::M_none )
        {
                pshader << "\t // Shader includes alpha test:\n";
                double ref = key.alpha_test_ref;
                switch ( key.alpha_test_mode )
                {
                case RenderAttrib::M_never:
                        pshader << "\t discard;\n";
                        break;
                case RenderAttrib::M_less:
                        pshader << "\t if (result.a >= " << ref << ") discard;\n";
                        break;
                case RenderAttrib::M_equal:
                        pshader << "\t if (result.a != " << ref << ") discard;\n";
                        break;
                case RenderAttrib::M_less_equal:
                        pshader << "\t if (result.a > " << ref << ") discard;\n";
                        break;
                case RenderAttrib::M_greater:
                        pshader << "\t if (result.a <= " << ref << ") discard;\n";
                        break;
                case RenderAttrib::M_not_equal:
                        pshader << "\t if (result.a == " << ref << ") discard;\n";
                        break;
                case RenderAttrib::M_greater_equal:
                        pshader << "\t if (result.a < " << ref << ") discard;\n";
                        break;
                case RenderAttrib::M_none:
                case RenderAttrib::M_always:
                        break;
                }
        }

        if ( key.outputs & AuxBitplaneAttrib::ABO_glow )
        {
                if ( key.texture_flags & shaderinfo_t::TEXTUREFLAGS_GLOWMAP )
                {
                        pshader << "\t result.a = tex" << map_index_glow << ".a;\n";
                }
                else
                {
                        pshader << "\t result.a = 0.5;\n";
                }
        }
        if ( key.outputs & AuxBitplaneAttrib::ABO_aux_glow )
        {
                if ( key.texture_flags & shaderinfo_t::TEXTUREFLAGS_GLOWMAP )
                {
                        pshader << "\t o_aux.a = tex" << map_index_glow << ".a;\n";
                }
                else
                {
                        pshader << "\t o_aux.a = 0.5;\n";
                }
        }

        if ( have_specular )
        {
                if ( key.material_flags & Material::F_specular )
                {
                        pshader << "\t tot_specular *= p3d_Material.specular;\n";
                }
                if ( key.texture_flags & shaderinfo_t::TEXTUREFLAGS_GLOSSMAP )
                {
                        pshader << "\t tot_specular *= tex" << map_index_gloss << ".a;\n";
                }
                pshader << "\t result.rgb += tot_specular.rgb;\n";
        }

        // Apply fog.
        if ( key.fog_mode != 0 )
        {
                Fog::Mode fog_mode = ( Fog::Mode )( key.fog_mode - 1 );
                switch ( fog_mode )
                {
                case Fog::M_linear:
                        pshader << "\t result.rgb = lerp(attr_fogcolor.rgb, result.rgb, clamp((attr_fog.z - l_hpos.z) * attr_fog.w, 0, 1));\n";
                        break;
                case Fog::M_exponential: // 1.442695f = 1 / log(2)
                        pshader << "\t result.rgb = lerp(attr_fogcolor.rgb, result.rgb, clamp(exp2(attr_fog.x * l_hpos.z * -1.442695f), 0, 1));\n";
                        break;
                case Fog::M_exponential_squared:
                        pshader << "\t result.rgb = lerp(attr_fogcolor.rgb, result.rgb, clamp(exp2(attr_fog.x * attr_fog.x * l_hpos.z * l_hpos.z * -1.442695f), 0, 1));\n";
                        break;
                }
        }

        // The multiply is a workaround for a radeon driver bug.  It's annoying as
        // heck, since it produces an extra instruction.
        pshader << "\t o_color = result * 1.0000001;\n";
        if ( key.alpha_test_mode != RenderAttrib::M_none )
        {
                pshader << "\t // Shader subsumes normal alpha test.\n";
        }
        if ( key.disable_alpha_write )
        {
                pshader << "\t // Shader disables alpha write.\n";
        }
        pshader << "}\n";

#if 0
        if ( key.texture_flags & shaderinfo_t::TEXTUREFLAGS_NORMALMAP )
        {
                VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();

                std::ostream *ost;

                ost = vfs->open_write_file( Filename( "bsp_generated_v.glsl" ), false, false );
                ost->clear();
                *ost << vshader.str();
                vfs->close_write_file( ost );

                ost = vfs->open_write_file( Filename( "bsp_generated_f.glsl" ), false, false );
                ost->clear();
                *ost << pshader.str();
                vfs->close_write_file( ost );
        }
#endif

        PT( Shader ) shader = Shader::make( Shader::SL_GLSL, vshader.str(), pshader.str() );
        nassertr( shader != nullptr, nullptr );

        CPT( RenderAttrib ) shattr = ShaderAttrib::make( shader );
        if ( key.alpha_test_mode != RenderAttrib::M_none )
        {
                shattr = DCAST( ShaderAttrib, shattr )->set_flag( ShaderAttrib::F_subsume_alpha_test, true );
        }
        if ( key.disable_alpha_write )
        {
                shattr = DCAST( ShaderAttrib, shattr )->set_flag( ShaderAttrib::F_disable_alpha_write, true );
        }

        CPT( ShaderAttrib ) attr = DCAST( ShaderAttrib, shattr );
        _shader_cache[key] = attr;
        return attr;
}

void BSPShaderGenerator::analyze_renderstate( shaderinfo_t &key, CPT( RenderState ) rs )
{
        // verify_enforce_attrib_lock();
        const AuxBitplaneAttrib *aux_bitplane;
        rs->get_attrib_def( aux_bitplane );
        key.outputs = aux_bitplane->get_outputs();

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
        if ( key.outputs & AuxBitplaneAttrib::ABO_glow )
        {
                if ( have_alpha_blend )
                {
                        key.outputs &= ~AuxBitplaneAttrib::ABO_glow;
                        key.disable_alpha_write = true;
                }
                else if ( have_alpha_test )
                {
                        // Subsume the alpha test in our shader.
                        key.alpha_test_mode = alpha_test->get_mode();
                        key.alpha_test_ref = alpha_test->get_reference_alpha();
                }
        }

        if ( have_alpha_blend || have_alpha_test )
        {
                key.calc_primary_alpha = true;
        }

        // Determine whether or not vertex colors or flat colors are present.
        const ColorAttrib *color;
        rs->get_attrib_def( color );
        key.color_type = color->get_color_type();

        // Store the material flags (not the material values itself).
        const MaterialAttrib *material;
        rs->get_attrib_def( material );
        Material *mat = material->get_material();
        if ( mat != nullptr )
        {
                // The next time the Material flags change, the Material should cause the
                // states to be rehashed.
                key.material_flags = mat->get_flags();
                key.shade_model = mat->get_shade_model();
        }

        // See if there is a normal map, height map, gloss map, or glow map.  Also
		// check if anything has TexGen.

        const TextureAttrib *texture;
        rs->get_attrib_def( texture );
        const TexGenAttrib *tex_gen;
        rs->get_attrib_def( tex_gen );
        const TexMatrixAttrib *tex_matrix;
        rs->get_attrib_def( tex_matrix );

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

                shaderinfo_t::textureinfo_t info;
                info.type = tex->get_texture_type();
                info.mode = stage->get_mode();
                info.flags = 0;
                info.combine_rgb = 0u;
                info.combine_alpha = 0u;

                // While we look at the mode, determine whether we need to change the mode
                // in order to reflect disabled features.
                switch ( info.mode )
                {
                case TextureStage::M_modulate:
                {
                        Texture::Format format = tex->get_format();
                        if ( format != Texture::F_alpha )
                        {
                                info.flags |= shaderinfo_t::TEXTUREFLAGS_HAS_RGB;
                        }
                        if ( Texture::has_alpha( format ) )
                        {
                                info.flags |= shaderinfo_t::TEXTUREFLAGS_HAS_ALPHA;
                        }
                }
                break;

                case TextureStage::M_modulate_glow:
                        info.flags = shaderinfo_t::TEXTUREFLAGS_GLOWMAP;
                        break;

                case TextureStage::M_modulate_gloss:
                        info.flags = shaderinfo_t::TEXTUREFLAGS_GLOSSMAP;
                        break;

                case TextureStage::M_normal_height:
                        if ( parallax_mapping_samples == 0 )
                        {
                                info.mode = TextureStage::M_normal;
                        }
                        else
                        {
                                info.flags = shaderinfo_t::TEXTUREFLAGS_NORMALMAP | shaderinfo_t::TEXTUREFLAGS_HEIGHTMAP;
                        }
                        break;

                case TextureStage::M_normal_gloss:
                        info.flags = shaderinfo_t::TEXTUREFLAGS_NORMALMAP | shaderinfo_t::TEXTUREFLAGS_GLOSSMAP;
                        break;

                case TextureStage::M_combine:
                        // If we have this rare, special mode, we encode all these extra
                        // parameters as flags to prevent bloating the shader key.
                        info.flags |= (uint32_t)stage->get_combine_rgb_mode() << shaderinfo_t::TEXTUREFLAGS_COMBINE_RGB_MODE_SHIFT;
                        info.flags |= (uint32_t)stage->get_combine_alpha_mode() << shaderinfo_t::TEXTUREFLAGS_COMBINE_ALPHA_MODE_SHIFT;
                        if ( stage->get_rgb_scale() == 2 )
                        {
                                info.flags |= shaderinfo_t::TEXTUREFLAGS_RGB_SCALE_2;
                        }
                        if ( stage->get_rgb_scale() == 4 )
                        {
                                info.flags |= shaderinfo_t::TEXTUREFLAGS_RGB_SCALE_4;
                        }
                        if ( stage->get_alpha_scale() == 2 )
                        {
                                info.flags |= shaderinfo_t::TEXTUREFLAGS_ALPHA_SCALE_2;
                        }
                        if ( stage->get_alpha_scale() == 4 )
                        {
                                info.flags |= shaderinfo_t::TEXTUREFLAGS_ALPHA_SCALE_4;
                        }
                        info.combine_rgb = PACK_COMBINE(
                                stage->get_combine_rgb_source0(), stage->get_combine_rgb_operand0(),
                                stage->get_combine_rgb_source1(), stage->get_combine_rgb_operand1(),
                                stage->get_combine_rgb_source2(), stage->get_combine_rgb_operand2() );
                        info.combine_alpha = PACK_COMBINE(
                                stage->get_combine_alpha_source0(), stage->get_combine_alpha_operand0(),
                                stage->get_combine_alpha_source1(), stage->get_combine_alpha_operand1(),
                                stage->get_combine_alpha_source2(), stage->get_combine_alpha_operand2() );

                        if ( stage->uses_primary_color() )
                        {
                                info.flags |= shaderinfo_t::TEXTUREFLAGS_USES_PRIMARY_COLOR;
                        }
                        if ( stage->uses_last_saved_result() )
                        {
                                info.flags |= shaderinfo_t::TEXTUREFLAGS_USES_LAST_SAVED_RESULT;
                        }
                        break;

                default:
                        break;
                }

                // In fact, perhaps this stage should be disabled altogether?
                bool skip = false;
                switch ( info.mode )
                {
                case TextureStage::M_normal:
                        info.flags = shaderinfo_t::TEXTUREFLAGS_NORMALMAP;
                        break;
                case TextureStage::M_glow:
                        info.flags = shaderinfo_t::TEXTUREFLAGS_GLOWMAP;
                        break;
                case TextureStage::M_gloss:
                        info.flags = shaderinfo_t::TEXTUREFLAGS_GLOSSMAP;
                        break;
                case TextureStage::M_height:
                        if ( parallax_mapping_samples > 0 )
                        {
                                info.flags = shaderinfo_t::TEXTUREFLAGS_HEIGHTMAP;
                        }
                        else
                        {
                                skip = true;
                        }
                        break;
                default:
                        break;
                }
                // We can't just drop a disabled slot from the list, since then the
                // indices for the texture stages will no longer match up.  So we keep it,
                // but set it to a noop state to indicate that it should be skipped.
                if ( skip )
                {
                        info.type = Texture::TT_1d_texture;
                        info.mode = TextureStage::M_modulate;
                        info.flags = 0;
                        key.textures.push_back( info );
                        continue;
                }

                // Check if this state has a texture matrix to transform the texture
                // coordinates.
                if ( tex_matrix->has_stage( stage ) )
                {
                        CPT( TransformState ) transform = tex_matrix->get_transform( stage );
                        if ( !transform->is_identity() )
                        {
                                // Optimize for common case: if we only have a scale component, we
                                // can get away with fewer shader inputs and operations.
                                if ( transform->has_components() && !transform->has_nonzero_shear() &&
                                     transform->get_pos() == LPoint3::zero() &&
                                     transform->get_hpr() == LVecBase3::zero() )
                                {
                                        info.flags |= shaderinfo_t::TEXTUREFLAGS_HAS_SCALE;
                                }
                                else
                                {
                                        info.flags |= shaderinfo_t::TEXTUREFLAGS_HAS_MAT;
                                }
                        }
                }

                if ( tex_gen->has_stage( stage ) )
                {
                        info.texcoord_name = nullptr;
                        info.gen_mode = tex_gen->get_mode( stage );

                        if ( info.gen_mode == TexGenAttrib::M_eye_sphere_map )
                        {
                                info.flags |= shaderinfo_t::TEXTUREFLAGS_SPHEREMAP;
                        }
                }
                else
                {
                        info.texcoord_name = stage->get_texcoord_name();
                        info.gen_mode = TexGenAttrib::M_off;
                }

                // Does this stage require saving its result?
                if ( stage->get_saved_result() )
                {
                        info.flags |= shaderinfo_t::TEXTUREFLAGS_SAVED_RESULT;
                }

                // Does this stage need a texcolor_# input?
                if ( stage->uses_color() )
                {
                        info.flags |= shaderinfo_t::TEXTUREFLAGS_USES_COLOR;
                }

                key.textures.push_back( info );
                key.texture_flags |= info.flags;
        }

        // Does nothing use the saved result?  If so, don't bother saving it.
        if ( ( key.texture_flags & shaderinfo_t::TEXTUREFLAGS_USES_LAST_SAVED_RESULT ) == 0 &&
                ( key.texture_flags & shaderinfo_t::TEXTUREFLAGS_SAVED_RESULT ) != 0 )
        {

                pvector<shaderinfo_t::textureinfo_t>::iterator it;
                for ( it = key.textures.begin(); it != key.textures.end(); ++it )
                {
                        ( *it ).flags &= ~shaderinfo_t::TEXTUREFLAGS_SAVED_RESULT;
                }
                key.texture_flags &= ~shaderinfo_t::TEXTUREFLAGS_SAVED_RESULT;
        }

        const LightRampAttrib *light_ramp;
        if ( rs->get_attrib( light_ramp ) )
        {
                key.light_ramp = light_ramp;
        }

        // Check for clip planes.
        const ClipPlaneAttrib *clip_plane;
        rs->get_attrib_def( clip_plane );
        key.num_clip_planes = clip_plane->get_num_on_planes();

        // Check for fog.
        const FogAttrib *fog;
        if ( rs->get_attrib( fog ) && !fog->is_off() )
        {
                key.fog_mode = (int)fog->get_fog()->get_mode() + 1;
        }
}

static PStatCollector findsky_collector( "AmbientProbes:FindLight/Sky" );
static PStatCollector updatenode_collector( "AmbientProbes:UpdateNodes" );

using std::cos;
using std::sin;

#define CHANGED_EPSILON 0.1
#define GARBAGECOLLECT_TIME 10.0
//#define VISUALIZE_AMBPROBES

static light_t *dummy_light = new light_t;

CPT( ShaderAttrib ) AmbientProbeManager::_identity_shattr = nullptr;
CPT( Shader ) AmbientProbeManager::_shader = nullptr;

AmbientProbeManager::AmbientProbeManager() :
        _loader( nullptr ),
        _sunlight( nullptr )
{
        dummy_light->leaf = 0;
        dummy_light->type = 0;
}

AmbientProbeManager::AmbientProbeManager( BSPLoader *loader ) :
        _loader( loader ),
        _sunlight( nullptr )
{
}

CPT( Shader ) AmbientProbeManager::get_shader()
{
        if ( _shader == nullptr )
        {
                _shader = Shader::load( Shader::SL_GLSL, Filename( "phase_14/models/shaders/bsp_dynamic_v.glsl" ),
                                        Filename( "phase_14/models/shaders/bsp_dynamic_f.glsl" ) );
        }

        return _shader;
}

CPT( ShaderAttrib ) AmbientProbeManager::get_identity_shattr()
{
        if ( _identity_shattr == nullptr )
        {
                CPT( ShaderAttrib ) shattr = DCAST( ShaderAttrib, ShaderAttrib::make( get_shader() ) );
                shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "ambient_cube", PTA_LVecBase3::empty_array( 6 ) ) ) );
                shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "num_locallights", PTA_int::empty_array( 1 ) ) ) );
                shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "locallight_type", PTA_int::empty_array( 2 ) ) ) );
                for ( int i = 0; i < MAXLIGHTS; i++ )
                {
                        std::stringstream ss;
                        ss << "locallight[" << i << "]";
                        std::string name = ss.str();
                        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( name + ".pos", LVector3( 0 ) ) ) );
                        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( name + ".direction", LVector4( 0 ) ) ) );
                        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( name + ".atten", LVector4( 0 ) ) ) );
                        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( name + ".color", LVector3( 1 ) ) ) );
                }

                _identity_shattr = shattr;
        }

        return _identity_shattr;
}

INLINE LVector3 angles_to_vector( const vec3_t &angles )
{
        return LVector3( cos( deg_2_rad(angles[0]) ) * cos( deg_2_rad(angles[1]) ),
                         sin( deg_2_rad(angles[0]) ) * cos( deg_2_rad(angles[1]) ),
                         sin( deg_2_rad(angles[1]) ) );
}

INLINE int lighttype_from_classname( const char *classname )
{
        if ( !strncmp( classname, "light_environment", 18 ) )
        {
                return LIGHTTYPE_SUN;
        }
        else if ( !strncmp( classname, "light_spot", 11 ) )
        {
                return LIGHTTYPE_SPOT;
        }
        else if ( !strncmp( classname, "light", 5 ) )
        {
                return LIGHTTYPE_POINT;
        }

        return LIGHTTYPE_POINT;
}

void AmbientProbeManager::process_ambient_probes()
{

#ifdef VISUALIZE_AMBPROBES
        if ( !_vis_root.is_empty() )
        {
                _vis_root.remove_node();
        }
        _vis_root = _loader->_result.attach_new_node( "visAmbProbesRoot" );
#endif

        _light_pvs.clear();
        _light_pvs.resize( _loader->_bspdata->dmodels[0].visleafs + 1 );
        _all_lights.clear();

        // Build light data structures
        for ( int entnum = 0; entnum < _loader->_bspdata->numentities; entnum++ )
        {
                entity_t *ent = _loader->_bspdata->entities + entnum;
                const char *classname = ValueForKey( ent, "classname" );
                if ( !strncmp( classname, "light", 5 ) )
                {
                        PT( light_t ) light = new light_t;

                        vec3_t pos;
                        GetVectorForKey( ent, "origin", pos );
                        VectorCopy( pos, light->pos );
                        VectorScale( light->pos, 1 / 16.0, light->pos );

                        light->leaf = _loader->find_leaf( light->pos );
                        light->color = color_from_value( ValueForKey( ent, "_light" ) ).get_xyz();
                        light->type = lighttype_from_classname( classname );

                        if ( light->type == LIGHTTYPE_SUN ||
                             light->type == LIGHTTYPE_SPOT)
                        {
                                vec3_t angles;
                                GetVectorForKey( ent, "angles", angles );
                                float pitch = FloatForKey( ent, "pitch" );
                                float temp = angles[1];
                                if ( !pitch )
                                {
                                        pitch = angles[0];
                                }
                                angles[1] = pitch;
                                angles[0] = temp;
                                VectorCopy( angles_to_vector( angles ), light->direction );
                                light->direction[3] = 0.0;
                        }

                        if ( light->type == LIGHTTYPE_SPOT ||
                             light->type == LIGHTTYPE_POINT )
                        {
                                float fade = FloatForKey( ent, "_fade" );
                                light->atten = LVector4( fade / 16.0, 0, 0, 0 );
                                if ( light->type == LIGHTTYPE_SPOT )
                                {
                                        float stopdot = FloatForKey( ent, "_cone" );
                                        if ( !stopdot )
                                        {
                                                stopdot = 10;
                                        }
                                        float stopdot2 = FloatForKey( ent, "_cone2" );
                                        if ( !stopdot2 )
                                        {
                                                stopdot2 = stopdot;
                                        }
                                        if ( stopdot2 < stopdot )
                                        {
                                                stopdot2 = stopdot;
                                        }

                                        stopdot = (float)std::cos( stopdot / 180 * Q_PI );
                                        stopdot2 = (float)std::cos( stopdot2 / 180 * Q_PI );

                                        light->atten[1] = stopdot;
                                        light->atten[2] = stopdot2;
                                }
                        }

                        _all_lights.push_back( light );
                        if ( light->type == LIGHTTYPE_SUN )
                        {
                                _sunlight = light;
                        }
                }
        }

        // Build light PVS
        for ( size_t lightnum = 0; lightnum < _all_lights.size(); lightnum++ )
        {
                light_t *light = _all_lights[lightnum];
                for ( int leafnum = 0; leafnum < _loader->_bspdata->dmodels[0].visleafs + 1; leafnum++ )
                {
                        if ( light->type != LIGHTTYPE_SUN &&
                             _loader->is_cluster_visible(light->leaf, leafnum) )
                        {
                                _light_pvs[leafnum].push_back( light );
                        }
                }
        }

        for ( size_t i = 0; i < _loader->_bspdata->leafambientindex.size(); i++ )
        {
                dleafambientindex_t *ambidx = &_loader->_bspdata->leafambientindex[i];
                dleaf_t *leaf = _loader->_bspdata->dleafs + i;
                _probes[i] = pvector<ambientprobe_t>();
                for ( int j = 0; j < ambidx->num_ambient_samples; j++ )
                {
                        dleafambientlighting_t *light = &_loader->_bspdata->leafambientlighting[ambidx->first_ambient_sample + j];
                        ambientprobe_t probe;
                        probe.leaf = i;
                        probe.pos = LPoint3( RemapValClamped( light->x, 0, 255, leaf->mins[0], leaf->maxs[0] ),
                                             RemapValClamped( light->y, 0, 255, leaf->mins[1], leaf->maxs[1] ),
                                             RemapValClamped( light->z, 0, 255, leaf->mins[2], leaf->maxs[2] ) );
                        probe.pos /= 16.0;
                        probe.cube = PTA_LVecBase3::empty_array( 6 );
                        for ( int k = 0; k < 6; k++ )
                        {
                                probe.cube.set_element( k, color_shift_pixel( &light->cube.color[k], _loader->_gamma ) );
                        }
#ifdef VISUALIZE_AMBPROBES
                        PT( TextNode ) tn = new TextNode( "visText" );
                        char text[40];
                        sprintf( text, "Ambient Sample %i\nLeaf %i", j, i );
                        tn->set_align( TextNode::A_center );
                        tn->set_text( text );
                        tn->set_text_color( LColor( 1, 1, 1, 1 ) );
                        NodePath smiley = _vis_root.attach_new_node( tn->generate() );
                        smiley.set_pos( probe.pos );
                        smiley.set_billboard_axis();
                        smiley.clear_model_nodes();
                        smiley.flatten_strong();
                        probe.visnode = smiley;
#endif
                        _probes[i].push_back( probe );
                }
        }
}

INLINE bool AmbientProbeManager::is_sky_visible( const LPoint3 &point )
{
        if ( _sunlight == nullptr )
        {
                return false;
        }

        findsky_collector.start();

        LPoint3 start( ( point + LPoint3( 0, 0, 0.05 ) ) * 16 );
        LPoint3 end = start - ( _sunlight->direction.get_xyz() * 10000 );
        Ray ray( start, end, LPoint3::zero(), LPoint3::zero() );
        FaceFinder finder( _loader->_bspdata );
        bool ret = !finder.find_intersection( ray );

        findsky_collector.stop();

        return ret;
}

INLINE bool AmbientProbeManager::is_light_visible( const LPoint3 &point, const light_t *light )
{
        findsky_collector.start();

        Ray ray( ( point + LPoint3( 0, 0, 0.05 ) ) * 16, light->pos * 16, LPoint3::zero(), LPoint3::zero() );
        FaceFinder finder( _loader->_bspdata );
        bool ret = !finder.find_intersection( ray );

        findsky_collector.stop();

        return ret;
}

INLINE void AmbientProbeManager::garbage_collect_cache()
{
        for ( auto itr = _pos_cache.cbegin(); itr != _pos_cache.cend(); )
        {
                if ( itr->first.was_deleted() )
                {
                        _pos_cache.erase( itr++ );
                }
                else
                {
                        itr++;
                }
        }

        for ( auto itr = _node_data.cbegin(); itr != _node_data.cend(); )
        {
                if ( itr->first.was_deleted() )
                {
                        _node_data.erase( itr++ );
                } else
                {
                        itr++;
                }
        }
}

void AmbientProbeManager::consider_garbage_collect()
{
        double now = ClockObject::get_global_clock()->get_frame_time();
        if ( now - _last_garbage_collect_time >= GARBAGECOLLECT_TIME )
        {
                garbage_collect_cache();
                _last_garbage_collect_time = now;
        }
}

INLINE CPT( ShaderAttrib ) set_shader_inputs( CPT( ShaderAttrib ) shattr, const nodeshaderinput_t *input )
{
        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "ambient_cube", input->ambient_cube ) ) );
        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "light_count", input->light_count ) ) );
        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "light_type", input->light_type ) ) );
        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "light_pos", input->light_pos ) ) );
        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "light_color", input->light_color ) ) );
        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "light_direction", input->light_direction ) ) );
        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "light_atten", input->light_atten ) ) );
        return shattr;
}

void AmbientProbeManager::generate_shaders( PandaNode *node, CPT( RenderState ) net_state, bool first, 
                                            const nodeshaderinput_t *input )
{

        if ( node->is_of_type( GeomNode::get_class_type() ) )
        {
                PT( GeomNode ) gn = DCAST( GeomNode, node );
                for ( int i = 0; i < gn->get_num_geoms(); i++ )
                {
                        CPT( RenderState ) geom_state = gn->get_geom_state( i );
                        CPT( RenderState ) net_geom_state = net_state->compose( geom_state );

                        if ( net_state->has_attrib( ShaderAttrib::get_class_slot() ) )
                        {
                                const ShaderAttrib *shattr = DCAST( ShaderAttrib, net_state->get_attrib( ShaderAttrib::get_class_slot() ) );
                                if ( !shattr->auto_shader() )
                                {
                                        // Auto shader not enabled
                                        continue;
                                }
                        }
                        else
                        {
                                continue;
                        }

                        UpdateSeq &seq = _loader->_generated_shader_seq;

                        if ( ( net_geom_state->_generated_shader == nullptr || net_geom_state->_generated_shader_seq != seq) ||
                             ( geom_state->_generated_shader == nullptr || geom_state->_generated_shader_seq != seq ) )
                        {
                                CPT( ShaderAttrib ) shattr = _shader_generator.generate_shader( net_geom_state );
                                shattr = set_shader_inputs( shattr, input );

                                net_geom_state->_generated_shader = shattr;
                                net_geom_state->_generated_shader_seq = seq;

                                geom_state = geom_state->set_attrib( shattr );
                                geom_state->_generated_shader = shattr;
                                geom_state->_generated_shader_seq = seq;
                                gn->set_geom_state( i, geom_state );
                        }
                }
        }
}

void AmbientProbeManager::update_node( PandaNode *node, CPT( TransformState ) curr_trans, CPT(RenderState) net_state )
{
        PStatTimer timer( updatenode_collector );

        if ( !node || !curr_trans )
        {
                return;
        }

        bool new_instance = false;
        if ( _node_data.find( node ) == _node_data.end() )
        {
                // This is a new node we have encountered.
                new_instance = true;
        }

        PT( nodeshaderinput_t ) input;
        if ( new_instance )
        {
                input = new nodeshaderinput_t;
                _node_data[node] = input;
                _pos_cache[node] = curr_trans;
        }
        else
        {
                input = _node_data.at( node );
        }

        if ( input == nullptr )
        {
                return;
        }

        generate_shaders( node, net_state, true, input );

        // Is it even necessary to update anything?
        CPT( TransformState ) prev_trans = _pos_cache.at( node );
        LVector3 pos_delta( 0 );
        if ( prev_trans != nullptr )
        {
                pos_delta = curr_trans->get_pos() - prev_trans->get_pos();
        }
        else
        {
                _pos_cache[node] = curr_trans;
        }

        if ( pos_delta.length() >= EQUAL_EPSILON || new_instance )
        {
                LPoint3 curr_net = curr_trans->get_pos();
                int leaf_id = _loader->find_leaf( curr_net );

                // Update ambient cube
                if ( _probes[leaf_id].size() > 0 )
                {
                        ambientprobe_t *sample = find_closest_sample( leaf_id, curr_net );

#ifdef VISUALIZE_AMBPROBES
                        for ( size_t j = 0; j < _probes[leaf_id].size(); j++ )
                        {
                                _probes[leaf_id][j].visnode.set_color_scale( LColor( 0, 0, 1, 1 ), 1 );
                        }
                        if ( !sample->visnode.is_empty() )
                        {
                                sample->visnode.set_color_scale( LColor( 0, 1, 0, 1 ), 1 );
                        }
#endif
                        for ( int i = 0; i < 6; i++ )
                        {
                                input->ambient_cube[i] = sample->cube[i];
                        }
                }

                // Update local light sources
                pvector<light_t *> locallights = _light_pvs[leaf_id];
                // Sort local lights from closest to furthest distance from node, we will choose the two closest lights.
                std::sort( locallights.begin(), locallights.end(), [curr_net]( const light_t *a, const light_t *b )
                {
                        return ( a->pos - curr_net ).length() < ( b->pos - curr_net ).length();
                } );

                int sky_idx = -1;
                if ( is_sky_visible( curr_net ) )
                {
                        // If we hit the sky from current position, sunlight takes
                        // precedence over all other local light sources.
                        locallights.insert( locallights.begin(), _sunlight );
                        sky_idx = 0;
                }

                while ( locallights.size() < MAXLIGHTS )
                {
                        locallights.push_back( dummy_light );
                }
                size_t numlights = locallights.size();
                int lights_updated = 0;
                for ( size_t i = 0; i < numlights && lights_updated < MAXLIGHTS; i++ )
                {
                        light_t *light = locallights[i];
                        if ( i != sky_idx && !is_light_visible( curr_net, light ) )
                        {
                                // The light is occluded, don't add it.
                                continue;
                        }

                        input->light_type[lights_updated] = light->type;
                        input->light_pos[lights_updated] = light->pos;
                        input->light_color[lights_updated] = light->color;
                        input->light_direction[lights_updated] = light->direction;
                        input->light_atten[lights_updated] = light->atten;

                        lights_updated++;
                }
                input->light_count[0] = lights_updated;

                // Cache the last position.
                _pos_cache[node] = curr_trans;
        }
}

INLINE ambientprobe_t *AmbientProbeManager::find_closest_sample( int leaf_id, const LPoint3 &pos )
{
        pvector<ambientprobe_t> &probes = _probes[leaf_id];
        pvector<ambientprobe_t *> samples;
        samples.resize( probes.size() );
        for ( int i = 0; i < probes.size(); i++ )
        {
                samples[i] = &probes[i];
        }

        std::sort( samples.begin(), samples.end(), [pos]( const ambientprobe_t *a, const ambientprobe_t *b )
        {
                return ( a->pos - pos ).length() < ( b->pos - pos ).length();
        } );

        return samples[0];
}

void AmbientProbeManager::cleanup()
{
        _sunlight = nullptr;
        _pos_cache.clear();
        _node_data.clear();
        _probes.clear();
        _all_probes.clear();
        _light_pvs.clear();
        _all_lights.clear();
}

BSPShaderGenerator *AmbientProbeManager::get_shader_generator()
{
        return &_shader_generator;
}