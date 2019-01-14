/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file shader_generator.cpp
 * @author Brian Lach
 * @date October 22, 2018
 */

#include "shader_generator.h"
#include "pssmCameraRig.h"
#include "bsp_material.h"
#include "ambient_probes.h"
#include "cubemaps.h"
#include "aux_data_attrib.h"

#include <pStatTimer.h>
#include <config_pgraphnodes.h>
#include <material.h>
#include <auxBitplaneAttrib.h>
#include <pointLight.h>
#include <directionalLight.h>
#include <spotlight.h>
#include <sphereLight.h>
#include <fog.h>
#include <asyncTaskManager.h>
#include <configVariableInt.h>
#include <graphicsStateGuardian.h>
#include <graphicsEngine.h>
#include <windowProperties.h>
#include <camera.h>
#include <lightAttrib.h>
#include <materialAttrib.h>
#include <omniBoundingVolume.h>
#include <cullFaceAttrib.h>
#include <texturePool.h>
#include <antialiasAttrib.h>
#include <virtualFileSystem.h>
#include <alphaTestAttrib.h>
#include <colorBlendAttrib.h>
#include <clipPlaneAttrib.h>

using namespace std;

static PStatCollector findmatshader_collector( "*:Munge:PSSMShaderGen:FindMatShader" );
static PStatCollector lookup_collector( "*:Munge:PSSMShaderGen:Lookup" );
static PStatCollector synthesize_collector( "*:Munge:PSSMShaderGen:Synthesize" );

ConfigVariableInt pssm_splits( "pssm-splits", 3 );
ConfigVariableInt pssm_size( "pssm-size", 1024 );
ConfigVariableBool want_pssm( "want-pssm", false );
ConfigVariableDouble depth_bias( "pssm-shadow-depth-bias", 0.001 );
ConfigVariableDouble normal_offset_scale( "pssm-normal-offset-scale", 1.0 );
ConfigVariableDouble softness_factor( "pssm-softness-factor", 1.0 );
ConfigVariableBool cache_shaders( "pssm-cache-shaders", true );
ConfigVariableBool normal_offset_uv_space( "pssm-normal-offset-uv-space", true );

TypeHandle PSSMShaderGenerator::_type_handle;
PT( Texture ) PSSMShaderGenerator::_identity_cubemap = nullptr;

NotifyCategoryDef( bspShaderGenerator, "" );

PSSMShaderGenerator::PSSMShaderGenerator( GraphicsStateGuardian *gsg, const NodePath &camera, const NodePath &render ) :
        ShaderGenerator( gsg ),
        _gsg( gsg ),
        _update_task( new GenericAsyncTask( "PSSMShaderGenerator_update_pssm", update_pssm, this ) ),
        _pssm_rig( new PSSMCameraRig( pssm_splits ) ),
        _camera( camera ),
        _render( render ),
        _sun_vector( 0 ),
        _pssm_split_texture_array( nullptr ),
        _pssm_layered_buffer( nullptr ),
        _sunlight( NodePath() ),
        _has_shadow_sunlight( false )
{
        // Shadows need to be updated before literally anything else.
        // Any RTT of the main scene should happen after shadows are updated.
        _update_task->set_sort( -10000 );
        _pssm_rig->set_use_stable_csm( true );
        _pssm_rig->set_sun_distance( 400.0 );
        _pssm_rig->set_pssm_distance( 200.0 );
        _pssm_rig->set_resolution( pssm_size );
        _pssm_rig->set_use_fixed_film_size( true );
        
        if ( want_pssm )
        {
                _pssm_rig->reparent_to( _render );
                _pssm_split_texture_array = new Texture( "pssmSplitTextureArray" );
                _pssm_split_texture_array->setup_2d_texture_array( pssm_size, pssm_size, pssm_splits, Texture::T_float, Texture::F_depth_component32 );
                _pssm_split_texture_array->set_clear_color( LVecBase4( 1.0 ) );
                _pssm_split_texture_array->set_wrap_u( SamplerState::WM_clamp );
                _pssm_split_texture_array->set_wrap_v( SamplerState::WM_clamp );
                _pssm_split_texture_array->set_border_color( LColor( 1.0 ) );
                _pssm_split_texture_array->set_minfilter( SamplerState::FT_linear_mipmap_linear );
                _pssm_split_texture_array->set_magfilter( SamplerState::FT_linear );

                // Setup the buffer that this split shadow map will be rendered into.
                FrameBufferProperties fbp;
                fbp.set_depth_bits( shadow_depth_bits );
                WindowProperties props = WindowProperties::size( LVecBase2i( pssm_size ) );
                int flags = GraphicsPipe::BF_refuse_window;
                _pssm_layered_buffer = _gsg->get_engine()->make_output(
                        _gsg->get_pipe(), "pssmShadowBuffer", -10000, fbp, props,
                        flags, _gsg, _gsg->get_engine()->get_window( 0 )
                );

                // Using a geometry shader on the first PSSM split camera (the one that sees everything),
                // render to the individual textures in the array in a single render pass using geometry
                // shader cloning.
                _pssm_layered_buffer->add_render_texture( _pssm_split_texture_array, GraphicsOutput::RTM_bind_layered,
                                                GraphicsOutput::RTP_depth );

                CPT( RenderState ) state = RenderState::make_empty();
                // Apply a default texture in case a state the camera sees doesn't have one.
                //state = state->set_attrib( TextureAttrib::make( TexturePool::load_texture( "phase_14/maps/white.jpg" ) ) );
                state = state->set_attrib( LightAttrib::make_all_off(), 1 );
                state = state->set_attrib( MaterialAttrib::make_off(), 1 );

                CPT( RenderAttrib ) shattr = ShaderAttrib::make(
                        Shader::load( Shader::SL_GLSL,
                                      "phase_14/models/shaders/pssm_camera.vert.glsl",
                                      "phase_14/models/shaders/pssm_camera.frag.glsl",
                                      "phase_14/models/shaders/pssm_camera.geom.glsl" ) );
                shattr = DCAST( ShaderAttrib, shattr )->set_shader_input( ShaderInput( "split_mvps", _pssm_rig->get_mvp_array() ) );
                state = state->set_attrib( shattr, 1 );
                state = state->set_attrib( AntialiasAttrib::make( AntialiasAttrib::M_off ), 1 );
                state = state->set_attrib( TransparencyAttrib::make( TransparencyAttrib::M_dual ), 1 );
                state = state->set_attrib( CullFaceAttrib::make_reverse(), 1 );
                
                Camera *cam = DCAST( Camera, _pssm_rig->get_camera( 0 ).node() );
                cam->set_initial_state( state );
                cam->set_scene( _render );
                cam->set_cull_bounds( new OmniBoundingVolume() );
                cam->set_final( true );

                PT( DisplayRegion ) dr = _pssm_layered_buffer->make_display_region();
                dr->set_camera( _pssm_rig->get_camera( 0 ) );
                dr->set_sort( -10000 );
        }
}

/**
 * Adds a new shader that can be used by Materials.
 * The ShaderSpec class will setup the correct permutations
 * for the shader based on the RenderState.
 */
void PSSMShaderGenerator::add_shader( PT( ShaderSpec ) shader )
{
        _shaders[shader->get_name()] = shader;
}

void PSSMShaderGenerator::set_sun_light( const NodePath &np )
{
        _sunlight = np;

        DirectionalLight *dlight = DCAST( DirectionalLight, _sunlight.node() );
        _sun_vector = -dlight->get_direction();

        _has_shadow_sunlight = true;
}

void PSSMShaderGenerator::start_update()
{
        AsyncTaskManager *mgr = AsyncTaskManager::get_global_ptr();
        mgr->remove( _update_task );
        mgr->add( _update_task );
}

AsyncTask::DoneStatus PSSMShaderGenerator::update_pssm( GenericAsyncTask *task, void *data )
{
        if ( want_pssm )
        {
                PSSMShaderGenerator *self = (PSSMShaderGenerator *)data;
                if ( self->_sunlight.is_empty() || !self->_has_shadow_sunlight )
                {
                        self->_sunlight = NodePath();
                        self->_has_shadow_sunlight = false;
                        return AsyncTask::DS_cont;
                }
                self->_pssm_rig->update( self->_camera, self->_sun_vector );
        }
        
        return AsyncTask::DS_cont;
}

CPT( RenderAttrib ) apply_node_inputs( const RenderState *rs, CPT( RenderAttrib ) shattr )
{
        bool inputs_supplied = false;

        const AuxDataAttrib *ada;
        rs->get_attrib_def( ada );
        if ( ada->has_data() )
        {
                if ( ada->get_data()->is_exact_type( nodeshaderinput_t::get_class_type() ) )
                {
                        nodeshaderinput_t *bsp_node_input = DCAST( nodeshaderinput_t, ada->get_data() );
                        shattr = DCAST( ShaderAttrib, shattr )->set_shader_inputs(
                                {
                                        ShaderInput( "lightCount", bsp_node_input->light_count ),
                                        ShaderInput( "lightData", bsp_node_input->light_data ),
                                        ShaderInput( "lightData2", bsp_node_input->light_data2 ),
                                        ShaderInput( "lightTypes", bsp_node_input->light_type ),
                                        ShaderInput( "ambientCube", bsp_node_input->ambient_cube ),
                                        ShaderInput( "envmapSampler", bsp_node_input->cubemap_tex )
                                } );
                        inputs_supplied = true;
                }
        }
        if ( !inputs_supplied )
        {
                // Fill in default empty values so we don't crash.
                pvector<ShaderInput> inputs = {
                                ShaderInput( "lightCount", PTA_int::empty_array( 1 ) ),
                                ShaderInput( "lightData", PTA_LMatrix4::empty_array( MAX_TOTAL_LIGHTS ) ),
                                ShaderInput( "lightData2", PTA_LMatrix4::empty_array( MAX_TOTAL_LIGHTS ) ),
                                ShaderInput( "lightTypes", PTA_int::empty_array( MAX_TOTAL_LIGHTS ) ),
                                ShaderInput( "ambientCube", PTA_LVecBase3::empty_array( 6 ) )
                };
                // Do we have an envmap sampler already?
                if ( DCAST( ShaderAttrib, shattr )->get_shader_input( "envmapSampler" ) == ShaderInput::get_blank() )
                {
                        // Nope, give it the default envmap
                        inputs.push_back( ShaderInput( "envmapSampler", PSSMShaderGenerator::get_identity_cubemap() ) );
                }

                shattr = DCAST( ShaderAttrib, shattr )->set_shader_inputs( inputs );
        }

        return shattr;
}

CPT( ShaderAttrib ) PSSMShaderGenerator::synthesize_shader( const RenderState *rs,
                                                            const GeomVertexAnimationSpec &anim )
{
        findmatshader_collector.start();

        // First figure out which shader to use.
        // UnlitNoMat by default, unless specified by a Material.
        std::string shader_name = DEFAULT_SHADER;

        const BSPMaterialAttrib *mattr;
        rs->get_attrib_def( mattr );
        const BSPMaterial *mat = mattr->get_material();
        if ( mat )
        {
                shader_name = mat->get_shader();
        }

        if ( _shaders.find( shader_name ) == _shaders.end() )
        {
                // We haven't heard about this shader, they must've not called
                // add_shader().

                std::stringstream msg;
                msg << "Don't know about shader `" << shader_name << "` referenced by material `"
                        << mat->get_file().get_fullpath() << "`\n";
                msg << "Known shaders are:\n";
                for ( auto itr = _shaders.begin(); itr != _shaders.end(); ++itr )
                {
                        msg << "\t" << itr->first << "\n";
                }
                bspShaderGenerator_cat.warning()
                        << msg.str();

                findmatshader_collector.stop();
                return DCAST( ShaderAttrib, ShaderAttrib::make_default() );
        }

        findmatshader_collector.stop();

        ShaderPermutations permutations;
        ShaderSpec *spec;
        
        {
                PStatTimer timer( lookup_collector );

                spec = _shaders[shader_name];
                permutations = spec->setup_permutations( mat, rs, anim, this );

                if ( cache_shaders )
                {
                        auto itr = spec->_generated_shaders.find( permutations );
                        if ( itr != spec->_generated_shaders.end() )
                        {
                                CPT( ShaderAttrib ) shattr = itr->second;
                                shattr = DCAST( ShaderAttrib, apply_node_inputs( rs, shattr ) );

                                return shattr;
                        }
                }
                
        }
        
        synthesize_collector.start();

        stringstream defines;
        for ( auto itr = permutations.permutations.begin(); itr != permutations.permutations.end(); ++itr )
        {
                defines << "#define " << itr->first << " " << itr->second << "\n";
        }

        stringstream vshader, gshader, fshader;

        // Slip the defines into the shader source.
        if ( spec->_vertex.has )
        {
                vshader << spec->_vertex.before_defines
                        << "\n" << defines.str()
                        << spec->_vertex.after_defines;
        }
        if ( spec->_geom.has )
        {
                gshader << spec->_geom.before_defines
                        << "\n" << defines.str()
                        << spec->_geom.after_defines;
        }
        if ( spec->_pixel.has )
        {
                fshader << spec->_pixel.before_defines
                        << "\n" << defines.str()
                        << spec->_pixel.after_defines;
        }
        
        PT( Shader ) shader = Shader::make( Shader::SL_GLSL, vshader.str(), fshader.str(), gshader.str() );
        
        nassertr( shader != nullptr, nullptr );

        CPT( RenderAttrib ) shattr = ShaderAttrib::make( shader );

        // Add any inputs from the permutations.
        pvector<ShaderInput> perm_inputs;
        perm_inputs.reserve( permutations.inputs.size() );
        for ( auto itr = permutations.inputs.begin(); itr != permutations.inputs.end(); itr++ )
        {
                perm_inputs.push_back( itr->second.input );
        }
        shattr = DCAST( ShaderAttrib, shattr )->set_shader_inputs( perm_inputs );

        // Also any flags.
        for ( size_t i = 0; i < permutations.flags.size(); i++ )
        {
                shattr = DCAST( ShaderAttrib, shattr )->set_flag( permutations.flags[i], true );
        }
        
        CPT( ShaderAttrib ) attr = DCAST( ShaderAttrib, shattr );

        if ( cache_shaders )
                spec->_generated_shaders[permutations] = attr;

        shattr = apply_node_inputs( rs, shattr );
        attr = DCAST( ShaderAttrib, shattr );

        synthesize_collector.stop();

        return attr;
}

Texture *PSSMShaderGenerator::get_identity_cubemap()
{
        if ( !_identity_cubemap )
        {
                _identity_cubemap = TexturePool::load_cube_map( "phase_14/maps/defaultcubemap/defaultcubemap_#.jpg" );
        }

        return _identity_cubemap;
}