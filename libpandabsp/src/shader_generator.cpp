#include "shader_generator.h"
#include "pssmCameraRig.h"
#include "bsp_material.h"
#include "ambient_probes.h"

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

TypeHandle PSSMShaderGenerator::_type_handle;

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
        _pssm_rig->reparent_to( _render );
        if ( want_pssm )
        {
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

INLINE void apply_material_inputs( CPT( RenderAttrib ) shattr, BSPMaterial *bspmat )
{
        if ( bspmat != nullptr )
        {
                // Materials can also provide their own shader inputs.
                // Material inputs can override shader spec inputs.
                const pvector<ShaderInput> &mat_inps = bspmat->get_shader_inputs();
                size_t num_matinps = mat_inps.size();
                for ( size_t i = 0; i < num_matinps; i++ )
                {
                        shattr = DCAST( ShaderAttrib, shattr )->set_shader_input( mat_inps[i] );
                }
        }
}

CPT( ShaderAttrib ) PSSMShaderGenerator::synthesize_shader( const RenderState *rs,
                                                            const GeomVertexAnimationSpec &anim,
                                                            nodeshaderinput_t *bsp_node_input )
{
        CPT( ShaderAttrib ) shattr = synthesize_shader( rs, anim );
        CPT( RenderAttrib ) attr = shattr;

        if ( bsp_node_input != nullptr )
        {
                attr = DCAST( ShaderAttrib, attr )->set_shader_input( ShaderInput( "lightCount", bsp_node_input->light_count ) );
                attr = DCAST( ShaderAttrib, attr )->set_shader_input( ShaderInput( "lightData", bsp_node_input->light_data ) );
                attr = DCAST( ShaderAttrib, attr )->set_shader_input( ShaderInput( "lightTypes", bsp_node_input->light_type ) );
                attr = DCAST( ShaderAttrib, attr )->set_shader_input( ShaderInput( "ambientCube", bsp_node_input->ambient_cube ) );
        }

        return DCAST( ShaderAttrib, attr );
}

CPT( ShaderAttrib ) PSSMShaderGenerator::synthesize_shader( const RenderState *rs,
                                                            const GeomVertexAnimationSpec &anim )
{
        findmatshader_collector.start();

        // First figure out which shader to use.
        // VertexLitGeneric by default, unless specified by a Material.
        std::string shader_name = DEFAULT_SHADER;

        const MaterialAttrib *mattr;
        rs->get_attrib_def( mattr );
        Material *mat = mattr->get_material();
        BSPMaterial *bspmat = nullptr;
        if ( mat != nullptr && mat->is_of_type( BSPMaterial::get_class_type() ) )
        {
                bspmat = DCAST( BSPMaterial, mat );
                shader_name = bspmat->get_shader();
        }

        if ( _shaders.find( shader_name ) == _shaders.end() )
        {
                // We haven't heard about this shader, they must've not called
                // add_shader().

                std::stringstream msg;
                msg << "Don't know about shader `" << shader_name << "`\n";
                msg << "Known shaders are:\n";
                for ( auto itr = _shaders.begin(); itr != _shaders.end(); ++itr )
                {
                        msg << "\t" << itr->first << "\n";
                }
                bspShaderGenerator_cat.warning()
                        << msg.str();

                findmatshader_collector.stop();
                return nullptr;
        }

        findmatshader_collector.stop();

        ShaderSpec::Permutations permutations;
        ShaderSpec *spec;
        
        {
                PStatTimer timer( lookup_collector );

                spec = _shaders[shader_name];
                permutations = spec->setup_permutations( rs, anim, this );
#if 1
                auto itr = spec->_generated_shaders.find( permutations );
                if ( itr != spec->_generated_shaders.end() )
                {
                        CPT( RenderAttrib ) shattr = itr->second;
                        apply_material_inputs( shattr, bspmat );

                        lookup_collector.stop();
                        return DCAST( ShaderAttrib, shattr );
                }
#endif
        }
        
        synthesize_collector.start();

        VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();
        stringstream defines;
        for ( auto itr = permutations.permutations.begin(); itr != permutations.permutations.end(); ++itr )
        {
                defines << "#define " << itr->first << " " << itr->second << "\n";
        }

        size_t eofl;
        string vertext;
        stringstream vshader, gshader, fshader;

        if ( !spec->_vertex_file.empty() )
        {
                string vdata = vfs->read_file( spec->_vertex_file, true );
                eofl = vdata.find_first_of( '\n' );
                vertext = vdata.substr( 0, eofl );
                vdata = vdata.substr( eofl );
                vshader << vertext << "\n" << defines.str() << vdata;
                vfs->write_file( Filename( "generated_" + shader_name + ".vert.glsl" ), vshader.str(), true );
        }
        if ( !spec->_geom_file.empty() )
        {
                string gdata = vfs->read_file( spec->_geom_file, true );
                eofl = gdata.find_first_of( '\n' );
                vertext = gdata.substr( 0, eofl );
                gdata = gdata.substr( eofl );
                gshader << vertext << "\n" << defines.str() << gdata;
        }
        if ( !spec->_pixel_file.empty() )
        {
                string fdata = vfs->read_file( spec->_pixel_file, true );
                eofl = fdata.find_first_of( '\n' );
                vertext = fdata.substr( 0, eofl );
                fdata = fdata.substr( eofl );
                fshader << vertext << "\n" << defines.str() << fdata;
                vfs->write_file( Filename( "generated_" + shader_name + ".frag.glsl" ), fshader.str(), true );
        }
        
        PT( Shader ) shader = Shader::make( Shader::SL_GLSL, vshader.str(), fshader.str(), gshader.str() );
        
        nassertr( shader != nullptr, nullptr );

        CPT( RenderAttrib ) shattr = ShaderAttrib::make( shader );
        // Add any inputs from the shader spec.
        for ( size_t i = 0; i < permutations.inputs.size(); i++ )
        {
                shattr = DCAST( ShaderAttrib, shattr )->set_shader_input( permutations.inputs[i].input );
        }
        // Also any flags.
        for ( size_t i = 0; i < permutations.flags.size(); i++ )
        {
                shattr = DCAST( ShaderAttrib, shattr )->set_flag( permutations.flags[i], true );
        }
        
        CPT( ShaderAttrib ) attr = DCAST( ShaderAttrib, shattr );
#if 1
        spec->_generated_shaders[permutations] = attr;
#endif

        apply_material_inputs( shattr, bspmat );
        attr = DCAST( ShaderAttrib, shattr );

        synthesize_collector.stop();

        return attr;
}