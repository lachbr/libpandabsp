#include "commonfilters.h"
#include "pp_globals.h"

#include <auxBitplaneAttrib.h>
#include <texturePool.h>
#include <shaderPool.h>

static const string cartoon_body0 =
        "float4 cartoondelta = k_cartoonseparation * texpix_txaux.xwyw;"
        "float4 cartoon_c0 = tex2D(k_txaux, ";

static const string cartoon_body1 =
        " + cartoondelta.xy);"
        "float4 cartoon_c1 = tex2D(k_txaux, ";

static const string cartoon_body2 =
        " - cartoondelta.xy);"
        "float4 cartoon_c2 = tex2D(k_txaux, ";

static const string cartoon_body3 =
        " + cartoondelta.wz);"
        "float4 cartoon_c3 = tex2D(k_txaux, ";

static const string cartoon_body4 =
        " - cartoondelta.wz);"
        "float4 cartoon_mx = max(cartoon_c0, max(cartoon_c1, max(cartoon_c2, cartoon_c3)));"
        "float4 cartoon_mn = min(cartoon_c0, min(cartoon_c1, min(cartoon_c2, cartoon_c3)));"
        "float cartoon_thresh = saturate(dot(cartoon_mx - cartoon_mn, float4(3,3,0,0)) - 0.5);"
        "o_color = lerp(o_color, k_cartooncolor, cartoon_thresh);";

static const string ssao_body0 =
        "void vshader(float4 vtx_position : POSITION,"
        "             out float4 l_position : POSITION,"
        "             out float2 l_texcoord : TEXCOORD0,"
        "             out float2 l_texcoordD : TEXCOORD1, "
        "             out float2 l_texcoordN : TEXCOORD2, "
        "             uniform float4 texpad_depth, "
        "             uniform float4 texpad_normal, "
        "             uniform float4x4 mat_modelproj)"
        "{"
        "  l_position = mul(mat_modelproj, vtx_position); "
        "  l_texcoord = vtx_position.xz; "
        "  l_texcoordD = (vtx_position.xz * texpad_depth.xy) + texpad_depth.xy; "
        "  l_texcoordN = (vtx_position.xz * texpad_normal.xy) + texpad_normal.xy; "
        "}"
        ""
        "float3 sphere[16] = float3[](float3(0.53812504, 0.18565957, -0.43192), float3(0.13790712, 0.24864247, 0.44301823), float3(0.33715037, 0.56794053, -0.005789503), float3(-0.6999805, -0.04511441, -0.0019965635), float3(0.06896307, -0.15983082, -0.85477847), float3(0.056099437, 0.006954967, -0.1843352), float3(-0.014653638, 0.14027752, 0.0762037), float3(0.010019933, -0.1924225, -0.034443386), float3(-0.35775623, -0.5301969, -0.43581226), float3(-0.3169221, 0.106360726, 0.015860917), float3(0.010350345, -0.58698344, 0.0046293875), float3(-0.08972908, -0.49408212, 0.3287904), float3(0.7119986, -0.0154690035, -0.09183723), float3(-0.053382345, 0.059675813, -0.5411899), float3(0.035267662, -0.063188605, 0.54602677), float3(-0.47761092, 0.2847911, -0.0271716));"
        ""
        "void fshader(out float4 o_color : COLOR, "
        "             uniform float4 k_params1, "
        "             uniform float4 k_params2, "
        "             float2 l_texcoord : TEXCOORD0, "
        "             float2 l_texcoordD : TEXCOORD1, "
        "             float2 l_texcoordN : TEXCOORD2, "
        "             uniform sampler2D k_random : TEXUNIT0, "
        "             uniform sampler2D k_depth : TEXUNIT1, "
        "             uniform sampler2D k_normal : TEXUNIT2)"
        "{"
        "  float pixel_depth = tex2D(k_depth, l_texcoordD).a; "
        "  float3 pixel_normal = (tex2D(k_normal, l_texcoordN).xyz * 2.0 - 1.0); "
        "  float3 random_vector = normalize((tex2D(k_random, l_texcoord * 18.0 + pixel_depth + pixel_normal.xy).xyz * 2.0) - float3(1.0)).xyz; "
        "  float occlusion = 0.0; "
        "  float radius = k_params1.z / pixel_depth; "
        "  float depth_difference; "
        "  float3 sample_normal; "
        "  float3 ray; "
        "  for (int i = 0; i < ";

static const string ssao_body1 =
        "; ++i) {"
        "    ray = radius * reflect(sphere[i], random_vector);"
        "    sample_normal = (tex2D(k_normal, l_texcoordN + ray.xy).xyz * 2.0 - 1.0);"
        "    depth_difference = (pixel_depth - tex2D(k_depth, l_texcoordD + ray.xy).r);"
        "    occlusion += step(k_params2.y, depth_difference) * (1.0 - dot(sample_normal.xyz, pixel_normal)) * (1.0 - smoothstep(k_params2.y, k_params2.x, depth_difference));"
        "  }"
        "  o_color.rgb = 1.0 + (occlusion * k_params1.y);"
        "  o_color.a = 1.0;"
        "}";

CommonFilters::
CommonFilters( GraphicsOutput *win, NodePath &cam ) :
        _manager( win, cam ),
        _task( NULL ),
        _config_mode( false )
{

        cleanup();
}

void CommonFilters::
cleanup()
{
        _manager.cleanup();
        _textures.clear();
        _final_quad = NodePath();
        _bloom.clear();
        _blur.clear();
        _ssao.clear();
        if ( _task != NULL )
        {
                g_base->stop_task( _task );
                _task = NULL;
        }
}

void CommonFilters::
begin_config_mode()
{
        _config_mode = true;
}

void CommonFilters::
end_config_mode()
{
        _config_mode = false;
        reconfigure( true, FT_none );
}

bool CommonFilters::
has_configuration( FilterType filter ) const
{
        return ( _configuration.find( filter ) != _configuration.end() );
}

void CommonFilters::
update_clears()
{
        _manager.get_both_clears();
        //reconfigure(true, FT_none);
}

void CommonFilters::
reconfigure( bool full_rebuild, FilterType changed )
{
        if ( full_rebuild )
        {
                cleanup();

                if ( _configuration.size() == ( size_t )0 )
                {
                        return;
                }

                int aux_bits = 0;
                vector_string need_tex;
                need_tex.push_back( "color" );
                vector_string need_tex_coord;
                need_tex_coord.push_back( "color" );

                if ( has_configuration( FT_cartoon_ink ) )
                {
                        need_tex.push_back( "aux" );
                        aux_bits |= AuxBitplaneAttrib::ABO_aux_normal;
                        need_tex_coord.push_back( "aux" );
                }

                if ( has_configuration( FT_ambient_occlusion ) )
                {
                        need_tex.push_back( "depth" );
                        need_tex.push_back( "ssao0" );
                        need_tex.push_back( "ssao1" );
                        need_tex.push_back( "ssao2" );
                        need_tex.push_back( "aux" );
                        aux_bits |= AuxBitplaneAttrib::ABO_aux_normal;
                        need_tex_coord.push_back( "ssao2" );
                }

                if ( has_configuration( FT_blur_sharpen ) )
                {
                        need_tex.push_back( "blur0" );
                        need_tex.push_back( "blur1" );
                        need_tex_coord.push_back( "blur1" );
                }

                if ( has_configuration( FT_bloom ) )
                {
                        need_tex.push_back( "bloom0" );
                        need_tex.push_back( "bloom1" );
                        need_tex.push_back( "bloom2" );
                        need_tex.push_back( "bloom3" );
                        aux_bits |= AuxBitplaneAttrib::ABO_glow;
                        need_tex_coord.push_back( "bloom3" );
                }

                if ( has_configuration( FT_view_glow ) )
                {
                        aux_bits |= AuxBitplaneAttrib::ABO_glow;
                }

                if ( has_configuration( FT_volumetric_lighting ) )
                {
                        need_tex.push_back( ( ( VolumetricLightingConfig * )_configuration[FT_volumetric_lighting] )->_source );
                }

                for ( size_t i = 0; i < need_tex.size(); i++ )
                {
                        string type = need_tex[i];

                        _textures[type] = new Texture( "scene-" + type );
                        _textures[type]->set_wrap_u( SamplerState::WM_clamp );
                        _textures[type]->set_wrap_v( SamplerState::WM_clamp );
                }

                //if (!_final_quad.is_empty()) {
                // _final_quad.remove_node();
                //}
                _final_quad = _manager.render_scene_into( NULL, NULL, NULL, aux_bits, _textures );
                if ( _final_quad.is_empty() )
                {
                        cleanup();
                        return;
                }

                if ( has_configuration( FT_blur_sharpen ) )
                {
                        PT( Texture ) blur0 = _textures["blur0"];
                        PT( Texture ) blur1 = _textures["blur1"];
                        _blur.append( _manager.render_quad_into( 1, 2, 1, 0, blur0 ) );
                        _blur.append( _manager.render_quad_into( 1, 1, 1, 0, blur1 ) );
                        _blur[0].set_shader_input( "src", _textures["color"] );
                        _blur[0].set_shader( ShaderPool::load_shader( "shader/filter-blurx.sha" ) );
                        _blur[1].set_shader_input( "src", blur0 );
                        _blur[1].set_shader( ShaderPool::load_shader( "shader/filter-blury.sha" ) );
                }

                if ( has_configuration( FT_ambient_occlusion ) )
                {
                        PT( Texture ) ssao0 = _textures["ssao0"];
                        PT( Texture ) ssao1 = _textures["ssao1"];
                        PT( Texture ) ssao2 = _textures["ssao2"];
                        _ssao.append( _manager.render_quad_into( 1, 1, 1, 0, ssao0 ) );
                        _ssao.append( _manager.render_quad_into( 1, 2, 1, 0, ssao1 ) );
                        _ssao.append( _manager.render_quad_into( 1, 1, 1, 0, ssao2 ) );
                        _ssao[0].set_shader_input( "depth", _textures["depth"] );
                        _ssao[0].set_shader_input( "normal", _textures["aux"] );
                        _ssao[0].set_shader_input( "random", TexturePool::load_texture( "maps/random.rgb" ) );

                        stringstream ss;
                        ss << ssao_body0 << ( ( AmbientOcclusionConfig * )_configuration[FT_ambient_occlusion] )->_num_samples << ssao_body1;
                        _ssao[0].set_shader( Shader::make( ss.str(), Shader::SL_Cg ) );

                        _ssao[1].set_shader_input( "src", ssao0 );
                        _ssao[1].set_shader( ShaderPool::load_shader( "shader/filter-blurx.sha" ) );
                        _ssao[2].set_shader_input( "src", ssao1 );
                        _ssao[2].set_shader( ShaderPool::load_shader( "shader/filter-blury.sha" ) );

                }

                if ( has_configuration( FT_bloom ) )
                {
                        BloomConfig *bconf = ( BloomConfig * )_configuration[FT_bloom];
                        PT( Texture ) bloom0 = _textures["bloom0"];
                        PT( Texture ) bloom1 = _textures["bloom1"];
                        PT( Texture ) bloom2 = _textures["bloom2"];
                        PT( Texture ) bloom3 = _textures["bloom3"];

                        int scale = 2;
                        string downsampler = "shader/filter-copy.sha";
                        if ( bconf->_size == S_large )
                        {
                                scale = 8;
                                downsampler = "shader/filter-down4.sha";
                        }
                        else if ( bconf->_size = S_medium )
                        {
                                scale = 4;
                        }

                        _bloom.append( _manager.render_quad_into( 1, 2, scale, 0, bloom0 ) );
                        _bloom.append( _manager.render_quad_into( 1, scale, scale, 0, bloom1 ) );
                        _bloom.append( _manager.render_quad_into( 1, scale, scale, 0, bloom2 ) );
                        _bloom.append( _manager.render_quad_into( 1, scale, scale, 0, bloom3 ) );

                        _bloom[0].set_shader_input( "src", _textures["color"] );
                        _bloom[0].set_shader( ShaderPool::load_shader( "shader/filter-bloomi.sha" ) );
                        _bloom[1].set_shader_input( "src", bloom0 );
                        _bloom[1].set_shader( ShaderPool::load_shader( downsampler ) );
                        _bloom[2].set_shader_input( "src", bloom1 );
                        _bloom[2].set_shader( ShaderPool::load_shader( "shader/filter-bloomx.sha" ) );
                        _bloom[3].set_shader_input( "src", bloom2 );
                        _bloom[3].set_shader( ShaderPool::load_shader( "shader/filter-bloomy.sha" ) );
                }

                StringMap texcoords;
                StringMap texcoord_padding;

                for ( size_t i = 0; i < need_tex_coord.size(); i++ )
                {
                        string tex = need_tex_coord[i];

                        if ( _textures[tex]->get_auto_texture_scale() != ATS_none ||
                             has_configuration( FT_half_pixel_shift ) )
                        {

                                texcoords[tex] = "l_texcoord_" + tex;
                                texcoord_padding["l_texcoord_" + tex] = tex;

                        }
                        else
                        {
                                texcoords[tex] = "l_texcoord";
                                texcoord_padding["l_texcoord"] = "";

                        }
                }

                vector_string texcoord_sets;
                StringMap::iterator sm_itr;
                for ( sm_itr = texcoord_padding.begin(); sm_itr != texcoord_padding.end(); ++sm_itr )
                {
                        texcoord_sets.push_back( sm_itr->first );
                }

                string text;
                text = "//Cg\n";
                text += "void vshader(float4 vtx_position : POSITION,\n";
                text += "  out float4 l_position : POSITION,\n";

                for ( sm_itr = texcoord_padding.begin(); sm_itr != texcoord_padding.end(); ++sm_itr )
                {
                        string pad_tex = sm_itr->second;
                        if ( pad_tex.length() > 0 )
                        {
                                text += "  uniform float4 texpad_tx" + pad_tex + ",\n";
                                if ( has_configuration( FT_half_pixel_shift ) )
                                {
                                        text += "  uniform float4 texpix_tx" + pad_tex + ",\n";
                                }
                        }
                }

                for ( size_t i = 0; i < texcoord_sets.size(); i++ )
                {
                        string name = texcoord_sets[i];
                        stringstream ss;
                        ss << "  out float2 " << name << " : TEXCOORD" << i << ",\n";
                        text += ss.str();
                }

                text += "  uniform float4x4 mat_modelproj) {\n";
                text += "  l_position = mul(mat_modelproj, vtx_position);\n";

                for ( sm_itr = texcoord_padding.begin(); sm_itr != texcoord_padding.end(); ++sm_itr )
                {
                        string pad_tex = sm_itr->second;
                        string texcoord = sm_itr->first;
                        if ( pad_tex.length() == 0 )
                        {
                                text += "  " + texcoord + " = vtx_position.xz * float2(0.5, 0.5) + float2(0.5, 0.5);\n";
                        }
                        else
                        {
                                text += "  " + texcoord + " = (vtx_position.xz * texpad_tx" + pad_tex + ".xy) + texpad_tx" + pad_tex + ".xy;\n";

                                if ( has_configuration( FT_half_pixel_shift ) )
                                {
                                        text += "  " + texcoord + " += texpix_tx" + pad_tex + ".xy * 0.5;\n";
                                }
                        }
                }

                text += "}\n";

                text += "void fshader(";

                for ( size_t i = 0; i < texcoord_sets.size(); i++ )
                {
                        string name = texcoord_sets[i];
                        stringstream ss;
                        ss << "  float2 " << name << " : TEXCOORD" << i << ",\n";
                        text += ss.str();
                }

                for ( FilterManager::TextureMap::iterator titr = _textures.begin(); titr != _textures.end(); ++titr )
                {
                        text += "  uniform sampler2D k_tx" + titr->first + ",\n";
                }

                if ( has_configuration( FT_cartoon_ink ) )
                {
                        text += "  uniform float4 k_cartoonseparation,\n";
                        text += "  uniform float4 k_cartooncolor,\n";
                        text += "  uniform float4 texpix_txaux,\n";
                }

                if ( has_configuration( FT_blur_sharpen ) )
                {
                        text += "  uniform float4 k_blurval,\n";
                }

                if ( has_configuration( FT_volumetric_lighting ) )
                {
                        text += "  uniform float4 k_casterpos,\n";
                        text += "  uniform float4 k_vlparams,\n";
                }

                text += "  out float4 o_color : COLOR) {\n";
                text += "  o_color = tex2D(k_txcolor, " + texcoords["color"] + ");\n";

                if ( has_configuration( FT_cartoon_ink ) )
                {
                        string aux = texcoords["aux"];
                        text += cartoon_body0 + aux;
                        text += cartoon_body1 + aux;
                        text += cartoon_body2 + aux;
                        text += cartoon_body3 + aux;
                        text += cartoon_body4;
                }

                if ( has_configuration( FT_ambient_occlusion ) )
                {
                        text += "  o_color *= tex2D(k_txssao2, " + texcoords["ssao2"] + ").r;\n";
                }

                if ( has_configuration( FT_blur_sharpen ) )
                {
                        text += "  o_color = lerp(tex2D(k_txblur1, " + texcoords["blur1"] + "), o_color, k_blurval.x);\n";
                }

                if ( has_configuration( FT_bloom ) )
                {
                        text += "  o_color = saturate(o_color);\n";
                        text += "  float4 bloom = 0.5 * tex2D(k_txbloom3, " + texcoords["bloom3"] + ");\n";
                        text += "  o_color = 1 - ((1 - bloom) * (1 - o_color));\n";
                }

                if ( has_configuration( FT_view_glow ) )
                {
                        text += "  o_color.r = o_color.a;\n";
                }

                if ( has_configuration( FT_volumetric_lighting ) )
                {
                        VolumetricLightingConfig *vconf = ( VolumetricLightingConfig * )_configuration[FT_volumetric_lighting];

                        text += "  float decay = 1.0f;\n";
                        text += "  float2 curcoord = " + texcoords["color"] + ";\n";
                        text += "  float2 lightdir = curcoord - k_casterpos.xy;\n";
                        text += "  lightdir *= k_vlparams.x;\n";
                        text += "  half4 sample = tex2D(k_txcolor, curcoord);\n";
                        text += "  float3 vlcolor = sample.rgb * sample.a;\n";

                        stringstream ss;
                        ss   << "  for (int i = 0; i < " << vconf->_num_samples << "; i++) {\n";
                        text += ss.str();
                        text += "    curcoord -= lightdir;\n";
                        text += "    sample = tex2D(k_tx" + vconf->_source + ", curcoord);\n";
                        text += "    sample *= sample.a * decay;\n";
                        text += "    vlcolor += sample.rgb;\n";
                        text += "    decay *= k_vlparams.y;\n";
                        text += "  }\n";
                        text += "  o_color += float4(vlcolor * k_vlparams.z, 1);\n";
                }

                if ( has_configuration( FT_gamma_adjust ) )
                {
                        PN_stdfloat gamma = ( ( GammaAdjustConfig * )_configuration[FT_gamma_adjust] )->_gamma;
                        if ( gamma == 0.5 )
                        {
                                text += "  o_color.rgb = sqrt(o_color.rgb);\n";
                        }
                        else if ( gamma == 2.0 )
                        {
                                text += "  o_color.rgb *= o_color.rgb;\n";
                        }
                        else if ( gamma != 1.0 )
                        {
                                stringstream ss;
                                ss << "  o_color.rgb = pow(o_color.rgb, " << gamma << ");\n";
                                text += ss.str();
                        }
                }

                if ( has_configuration( FT_inverted ) )
                {
                        text += "  o_color = float4(1, 1, 1, 1) - o_color;\n";
                }

                text += "}\n";

                _final_quad.set_shader( Shader::make( text, Shader::SL_Cg ) );
                for ( FilterManager::TextureMap::iterator itr = _textures.begin(); itr != _textures.end(); ++itr )
                {
                        _final_quad.set_shader_input( "tx" + itr->first, itr->second );
                }

                if ( _task != NULL )
                {
                        g_base->stop_task( _task );
                        _task = NULL;
                }
                _task = g_base->start_task( update_task, this, "common-filters-update" );

        }

        if ( changed == FT_cartoon_ink || full_rebuild )
        {
                if ( has_configuration( FT_cartoon_ink ) )
                {
                        CartoonInkConfig *c = ( CartoonInkConfig * )_configuration[FT_cartoon_ink];
                        _final_quad.set_shader_input( "cartoonseparation", LVecBase4( c->_separation, 0, c->_separation, 0 ) );
                        _final_quad.set_shader_input( "cartooncolor", c->_color );
                }
        }

        if ( changed == FT_blur_sharpen || full_rebuild )
        {
                if ( has_configuration( FT_blur_sharpen ) )
                {
                        BlurSharpenConfig *c = ( BlurSharpenConfig * )_configuration[FT_blur_sharpen];
                        _final_quad.set_shader_input( "blurval", LVecBase4( c->_amount, c->_amount, c->_amount, c->_amount ) );
                }
        }

        if ( changed == FT_bloom || full_rebuild )
        {
                if ( has_configuration( FT_bloom ) )
                {
                        BloomConfig *c = ( BloomConfig * )_configuration[FT_bloom];
                        PN_stdfloat intensity = c->_intensity * 3.0;
                        _bloom[0].set_shader_input( "blend", c->_blend[0], c->_blend[1], c->_blend[2], c->_blend[3] * 2.0 );
                        _bloom[0].set_shader_input( "trigger", c->_min_trigger, 1.0 / ( c->_max_trigger - c->_min_trigger ), 0.0, 0.0 );
                        _bloom[0].set_shader_input( "desat", c->_desat, c->_desat, c->_desat, c->_desat );
                        _bloom[3].set_shader_input( "intensity", intensity, intensity, intensity, intensity );
                }
        }

        if ( changed == FT_volumetric_lighting || full_rebuild )
        {
                if ( has_configuration( FT_volumetric_lighting ) )
                {
                        VolumetricLightingConfig *c = ( VolumetricLightingConfig * )_configuration[FT_volumetric_lighting];
                        PN_stdfloat tc_param = c->_density / ( PN_stdfloat )c->_num_samples;
                        _final_quad.set_shader_input( "vlparams", tc_param, c->_decay, c->_exposure, 0.0 );
                }
        }

        if ( changed == FT_ambient_occlusion || full_rebuild )
        {
                if ( has_configuration( FT_ambient_occlusion ) )
                {
                        AmbientOcclusionConfig *c = ( AmbientOcclusionConfig * )_configuration[FT_ambient_occlusion];
                        _ssao[0].set_shader_input( "params1", ( PN_stdfloat )c->_num_samples, ( PN_stdfloat ) - c->_amount / ( PN_stdfloat )c->_num_samples,
                                                   ( PN_stdfloat )c->_radius, ( PN_stdfloat )0 );
                        _ssao[0].set_shader_input( "params2", ( PN_stdfloat )c->_strength, ( PN_stdfloat )c->_falloff, ( PN_stdfloat )0, ( PN_stdfloat )0 );

                }
        }

        update();
}

AsyncTask::DoneStatus CommonFilters::
update_task( GenericAsyncTask *task, void *data )
{
        ( ( CommonFilters * )data )->update();
        return AsyncTask::DS_cont;
}

void CommonFilters::
update()
{
        if ( _configuration.find( FT_volumetric_lighting ) != _configuration.end() )
        {
                NodePath caster = ( ( VolumetricLightingConfig * )_configuration[FT_volumetric_lighting] )->_caster;
                LPoint2 caster_pos;
                DCAST( Camera, _manager.get_cam().node() )->get_lens()->project( caster.get_pos( _manager.get_cam() ), caster_pos );
                _final_quad.set_shader_input( "casterpos",
                                              LVecBase4( caster_pos.get_x() * 0.5 + 0.5, ( caster_pos.get_y() * 0.5 + 0.5 ), 0, 0 ) );
        }
}

#define DeleteConfig(classname, type)\
classname *oldconf = (classname *)_configuration[type];\
_configuration.erase(_configuration.find(type));\
delete oldconf;

void CommonFilters::
set_cartoon_ink( PN_stdfloat separation, LColor &color )
{
        bool full_rebuild = ( _configuration.find( FT_cartoon_ink ) == _configuration.end() );

        if ( !full_rebuild )
        {
                DeleteConfig( CartoonInkConfig, FT_cartoon_ink );
        }

        CartoonInkConfig *config = new CartoonInkConfig;
        config->_separation = separation;
        config->_color = color;
        _configuration[FT_cartoon_ink] = config;

        if ( !_config_mode )
        {
                reconfigure( full_rebuild, FT_cartoon_ink );
        }
}

void CommonFilters::
del_cartoon_ink()
{
        if ( _configuration.find( FT_cartoon_ink ) != _configuration.end() )
        {
                DeleteConfig( CartoonInkConfig, FT_cartoon_ink );
                if ( !_config_mode )
                {
                        reconfigure( true, FT_cartoon_ink );
                }
        }
}

void CommonFilters::
set_bloom( LColor &blend, PN_stdfloat min_trigger, PN_stdfloat max_trigger, PN_stdfloat desat,
           PN_stdfloat intensity, Size size )
{
        if ( size == S_off )
        {
                del_bloom();
                return;
        }

        bool full_rebuild = true;

        if ( _configuration.find( FT_cartoon_ink ) != _configuration.end() )
        {
                if ( ( ( BloomConfig * )_configuration[FT_bloom] )->_size == size )
                {
                        DeleteConfig( BloomConfig, FT_bloom );
                        full_rebuild = false;
                }
        }

        BloomConfig *config = new BloomConfig;
        config->_blend = blend;
        config->_max_trigger = max_trigger;
        config->_min_trigger = min_trigger;
        config->_desat = desat;
        config->_intensity = intensity;
        config->_size = size;

        _configuration[FT_bloom] = config;

        if ( !_config_mode )
        {
                reconfigure( full_rebuild, FT_bloom );
        }
}

void CommonFilters::
del_bloom()
{
        if ( _configuration.find( FT_bloom ) != _configuration.end() )
        {
                DeleteConfig( BloomConfig, FT_bloom );
                if ( !_config_mode )
                {
                        reconfigure( true, FT_bloom );
                }
        }
}

void CommonFilters::
set_half_pixel_shift()
{
        bool full_rebuild = ( _configuration.find( FT_half_pixel_shift ) == _configuration.end() );
        if ( !full_rebuild )
        {
                DeleteConfig( HalfPixelShiftConfig, FT_half_pixel_shift );
        }

        HalfPixelShiftConfig *config = new HalfPixelShiftConfig;
        config->_flag = true;

        _configuration[FT_half_pixel_shift] = config;

        if ( !_config_mode )
        {
                reconfigure( full_rebuild, FT_half_pixel_shift );
        }

}

void CommonFilters::
del_half_pixel_shift()
{
        if ( _configuration.find( FT_half_pixel_shift ) != _configuration.end() )
        {
                DeleteConfig( HalfPixelShiftConfig, FT_half_pixel_shift );
                if ( !_config_mode )
                {
                        reconfigure( true, FT_half_pixel_shift );
                }

        }
}

void CommonFilters::
set_view_glow()
{
        bool full_rebuild = ( _configuration.find( FT_view_glow ) != _configuration.end() );
        if ( !full_rebuild )
        {
                DeleteConfig( ViewGlowConfig, FT_view_glow );
        }

        ViewGlowConfig *config = new ViewGlowConfig;
        config->_flag = true;

        _configuration[FT_view_glow] = config;

        if ( !_config_mode )
        {
                reconfigure( full_rebuild, FT_view_glow );
        }
}

void CommonFilters::
del_view_glow()
{
        if ( _configuration.find( FT_view_glow ) != _configuration.end() )
        {
                DeleteConfig( ViewGlowConfig, FT_view_glow );
                if ( !_config_mode )
                {
                        reconfigure( true, FT_view_glow );
                }
        }
}

void CommonFilters::
set_inverted()
{
        bool full_rebuild = ( _configuration.find( FT_inverted ) != _configuration.end() );
        if ( !full_rebuild )
        {
                DeleteConfig( InvertedConfig, FT_inverted );
        }

        InvertedConfig *config = new InvertedConfig;
        config->_flag = true;

        _configuration[FT_inverted] = config;

        if ( !_config_mode )
        {
                reconfigure( full_rebuild, FT_inverted );
        }
}

void CommonFilters::
del_inverted()
{
        if ( _configuration.find( FT_inverted ) != _configuration.end() )
        {
                DeleteConfig( InvertedConfig, FT_inverted );
                if ( !_config_mode )
                {
                        reconfigure( true, FT_inverted );
                }
        }
}

void CommonFilters::
set_volumetric_lighting( NodePath &caster, int num_samples, PN_stdfloat density,
                         PN_stdfloat decay, PN_stdfloat exposure,
                         string source )
{

        bool full_rebuild = true;

        if ( _configuration.find( FT_volumetric_lighting ) != _configuration.end() )
        {
                VolumetricLightingConfig *oldconf = ( VolumetricLightingConfig * )_configuration[FT_volumetric_lighting];
                if ( oldconf->_source == source && oldconf->_num_samples == num_samples )
                {
                        DeleteConfig( VolumetricLightingConfig, FT_volumetric_lighting );
                        full_rebuild = false;
                }
        }

        VolumetricLightingConfig *config = new VolumetricLightingConfig;
        config->_caster = caster;
        config->_num_samples = num_samples;
        config->_density = density;
        config->_decay = decay;
        config->_exposure = exposure;
        config->_source = source;

        _configuration[FT_volumetric_lighting] = config;

        if ( !_config_mode )
        {
                reconfigure( full_rebuild, FT_volumetric_lighting );
        }
}

void CommonFilters::
del_volumetric_lighting()
{
        if ( _configuration.find( FT_volumetric_lighting ) != _configuration.end() )
        {
                DeleteConfig( VolumetricLightingConfig, FT_volumetric_lighting );
                if ( !_config_mode )
                {
                        reconfigure( true, FT_volumetric_lighting );
                }
        }
}

void CommonFilters::
set_blur_sharpen( PN_stdfloat amount )
{
        bool full_rebuild = ( _configuration.find( FT_blur_sharpen ) == _configuration.end() );
        if ( !full_rebuild )
        {
                DeleteConfig( BlurSharpenConfig, FT_blur_sharpen );
        }

        BlurSharpenConfig *config = new BlurSharpenConfig;
        config->_amount = amount;

        _configuration[FT_blur_sharpen] = config;

        if ( !_config_mode )
        {
                reconfigure( full_rebuild, FT_blur_sharpen );
        }
}

void CommonFilters::
del_blur_sharpen()
{
        if ( _configuration.find( FT_blur_sharpen ) != _configuration.end() )
        {
                DeleteConfig( BlurSharpenConfig, FT_blur_sharpen );
                if ( !_config_mode )
                {
                        reconfigure( true, FT_blur_sharpen );
                }
        }
}

void CommonFilters::
set_ambient_occlusion( int num_samples, PN_stdfloat radius, PN_stdfloat amount,
                       PN_stdfloat strength, PN_stdfloat falloff )
{
        bool full_rebuild = ( _configuration.find( FT_ambient_occlusion ) == _configuration.end() );
        if ( !full_rebuild )
        {
                full_rebuild = ( num_samples != ( ( AmbientOcclusionConfig * )_configuration[FT_ambient_occlusion] )->_num_samples );
                DeleteConfig( AmbientOcclusionConfig, FT_ambient_occlusion );
        }

        AmbientOcclusionConfig *config = new AmbientOcclusionConfig;
        config->_num_samples = num_samples;
        config->_radius = radius;
        config->_amount = amount;
        config->_strength = strength;
        config->_falloff = falloff;

        _configuration[FT_ambient_occlusion] = config;

        if ( !_config_mode )
        {
                reconfigure( full_rebuild, FT_ambient_occlusion );
        }

}

void CommonFilters::
del_ambient_occlusion()
{
        if ( _configuration.find( FT_ambient_occlusion ) != _configuration.end() )
        {
                DeleteConfig( AmbientOcclusionConfig, FT_ambient_occlusion );
                if ( !_config_mode )
                {
                        reconfigure( true, FT_ambient_occlusion );
                }

        }
}

void CommonFilters::
set_gamma_adjust( PN_stdfloat gamma )
{
        bool full_rebuild = ( _configuration.find( FT_gamma_adjust ) == _configuration.end() );
        if ( !full_rebuild )
        {
                DeleteConfig( GammaAdjustConfig, FT_gamma_adjust );
        }

        GammaAdjustConfig *config = new GammaAdjustConfig;
        config->_gamma = gamma;

        _configuration[FT_gamma_adjust] = config;

        if ( !_config_mode )
        {
                reconfigure( full_rebuild, FT_gamma_adjust );
        }
}

void CommonFilters::
del_gamma_adjust()
{
        if ( _configuration.find( FT_gamma_adjust ) != _configuration.end() )
        {
                DeleteConfig( GammaAdjustConfig, FT_gamma_adjust );
                if ( !_config_mode )
                {
                        reconfigure( true, FT_gamma_adjust );
                }
        }
}