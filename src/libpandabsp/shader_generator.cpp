/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
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
#include "bsploader.h"

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
#include <colorScaleAttrib.h>
#include <cullBinAttrib.h>
#include <lens.h>

using namespace std;

static LightMutex cubemap_mutex( "CubemapMutex" );
static LightMutex synthesize_mutex( "SynthesizeMutex" );

static PStatCollector findmatshader_collector( "*:Munge:BSPShaderGen:FindMatShader" );
static PStatCollector lookup_collector( "*:Munge:BSPShaderGen:Lookup" );
static PStatCollector synthesize_collector( "*:Munge:BSPShaderGen:CreateShader" );
static PStatCollector gen_perms_collector( "*:Munge:BSPShaderGen:SetupPermutations" );
static PStatCollector complete_perms_collector( "*:Munge:BSPShaderGen:CompletePermutations" );
static PStatCollector make_attrib_collector( "*:Munge:BSPShaderGen:SetupShaderAttrib" );

ConfigVariableInt pssm_splits( "pssm-splits", 3 );
ConfigVariableInt pssm_size( "pssm-size", 1024 );
ConfigVariableInt pssm_max_distance( "pssm-max-distance", 200 );
ConfigVariableInt pssm_sun_distance( "pssm-sun-distance", 400 );
ConfigVariableBool want_pssm( "want-pssm", false );
ConfigVariableDouble depth_bias( "pssm-shadow-depth-bias", 0.001 );
ConfigVariableDouble normal_offset_scale( "pssm-normal-offset-scale", 1.0 );
ConfigVariableDouble softness_factor( "pssm-softness-factor", 1.0 );
ConfigVariableBool cache_shaders( "pssm-cache-shaders", true );
ConfigVariableBool normal_offset_uv_space( "pssm-normal-offset-uv-space", true );
ConfigVariableColor ambient_light_identifier( "pssm-ambient-light-identifier", LColor( 0.5, 0.5, 0.5, 1 ) );
ConfigVariableColor ambient_light_min( "pssm-ambient-light-min", LColor( 0, 0, 0, 1 ) );
ConfigVariableDouble ambient_light_scale( "pssm-ambient-light-scale", 1.0 );

TypeHandle BSPShaderGenerator::_type_handle;
PT( Texture ) BSPShaderGenerator::_identity_cubemap = nullptr;

NotifyCategoryDef( bspShaderGenerator, "" );

BSPShaderGenerator::BSPShaderGenerator( GraphicsOutput *output, GraphicsStateGuardian *gsg, const NodePath &camera, const NodePath &render ) :
	ShaderGenerator( gsg ),
	_gsg( gsg ),
	_output( output ),
	_pssm_rig( new PSSMCameraRig( pssm_splits, this ) ),
	_camera( camera ),
	_render( render ),
	_sun_vector( 0 ),
	_pssm_split_texture_array( nullptr ),
	_pssm_layered_buffer( nullptr ),
	_sunlight( NodePath() ),
	_has_shadow_sunlight( false ),
	_shader_quality( SHADERQUALITY_HIGH ),
	_fog( nullptr )
{
	_pta_fogdata = PTA_LVecBase4f::empty_array( 2 );
	_exposure_adjustment = PTA_float::empty_array( 1 );
	_exposure_adjustment[0] = 1.0f;

        // Shadows need to be updated before literally anything else.
        // Any RTT of the main scene should happen after shadows are updated.
        _pssm_rig->set_use_stable_csm( true );
        _pssm_rig->set_sun_distance( pssm_sun_distance );
        _pssm_rig->set_pssm_distance( pssm_max_distance );
        _pssm_rig->set_resolution( pssm_size );
        _pssm_rig->set_use_fixed_film_size( true );

        //BSPLoader::get_global_ptr()->set_shader_generator( this );

	_planar_reflections = new PlanarReflections( this );

        if ( want_pssm )
        {
                _pssm_split_texture_array = new Texture( "pssmSplitTextureArray" );
                _pssm_split_texture_array->setup_2d_texture_array( pssm_size, pssm_size, pssm_splits, Texture::T_float, Texture::F_depth_component32 );
                _pssm_split_texture_array->set_clear_color( LVecBase4( 1.0 ) );
                _pssm_split_texture_array->set_wrap_u( SamplerState::WM_clamp );
                _pssm_split_texture_array->set_wrap_v( SamplerState::WM_clamp );
                _pssm_split_texture_array->set_border_color( LColor( 1.0 ) );
                _pssm_split_texture_array->set_minfilter( SamplerState::FT_linear );
                _pssm_split_texture_array->set_magfilter( SamplerState::FT_linear );
                _pssm_split_texture_array->set_anisotropic_degree( 0 );

                // Setup the buffer that this split shadow map will be rendered into.
                FrameBufferProperties fbp;
                fbp.clear();
                fbp.set_depth_bits( shadow_depth_bits );
                fbp.set_back_buffers( 0 );
                fbp.set_force_hardware( true );
                fbp.set_multisamples( 0 );
                fbp.set_color_bits( 0 );
                fbp.set_alpha_bits( 0 );
                fbp.set_stencil_bits( 0 );
                fbp.set_float_color( false );
                fbp.set_float_depth( true );
                fbp.set_stereo( false );
                fbp.set_accum_bits( 0 );
                fbp.set_aux_float( 0 );
                fbp.set_aux_rgba( 0 );
                fbp.set_aux_hrgba( 0 );
                fbp.set_coverage_samples( 0 );

                WindowProperties props = WindowProperties::size( LVecBase2i( pssm_size ) );
                int flags = GraphicsPipe::BF_refuse_window | GraphicsPipe::BF_can_bind_layered;
                _pssm_layered_buffer = _gsg->get_engine()->make_output(
                        _gsg->get_pipe(), "pssmShadowBuffer", -10000, fbp, props,
                        flags, _gsg, _gsg->get_engine()->get_window( 0 )
                );
                _pssm_layered_buffer->disable_clears();

                // Using a geometry shader on the first PSSM split camera (the one that sees everything),
                // render to the individual textures in the array in a single render pass using geometry
                // shader cloning.
                _pssm_layered_buffer->add_render_texture( _pssm_split_texture_array, GraphicsOutput::RTM_bind_layered,
                        GraphicsOutput::RTP_depth );

                CPT( RenderState ) state = RenderState::make_empty();
                state = state->set_attrib( LightAttrib::make_all_off(), 10 );
                state = state->set_attrib( MaterialAttrib::make_off(), 10 );
                state = state->set_attrib( FogAttrib::make_off(), 10 );
                state = state->set_attrib( ColorAttrib::make_off(), 10 );
                state = state->set_attrib( ColorScaleAttrib::make_off(), 10 );
                state = state->set_attrib( AntialiasAttrib::make( AntialiasAttrib::M_off ), 10 );
                state = state->set_attrib( TextureAttrib::make_all_off(), 10 );
                state = state->set_attrib( ColorBlendAttrib::make_off(), 10 );
                state = state->set_attrib( CullBinAttrib::make_default(), 10 );
                state = state->set_attrib( TransparencyAttrib::make( ( TransparencyAttrib::Mode )TransparencyAttrib::M_off ), 10 );
                state = state->set_attrib( CullFaceAttrib::make( CullFaceAttrib::M_cull_none ), 10 );

                // Automatically generate shaders for the shadow scene using the CSMRender shader.
                CPT( RenderAttrib ) shattr = ShaderAttrib::make();
                shattr = DCAST( ShaderAttrib, shattr )->set_shader_auto();
                state = state->set_attrib( shattr );
                state = state->set_attrib( BSPMaterialAttrib::make_override_shader( BSPMaterial::get_from_file(
                        "materials/engine/csm_shadow.mat"
                ) ) );

                Camera *maincam = DCAST( Camera, _pssm_rig->get_camera( 0 ).node() );
                maincam->set_initial_state( state );
                maincam->set_cull_bounds( new OmniBoundingVolume );

                // So we can be selective over what casts shadows
                for ( int i = 0; i < pssm_splits; i++ )
                {
                        Camera *cam = DCAST( Camera, _pssm_rig->get_camera( i ).node() );
                        cam->set_camera_mask( CAMERA_SHADOW );
                }

                PT( DisplayRegion ) dr = _pssm_layered_buffer->make_display_region();
                dr->disable_clears();
                dr->set_clear_depth_active( true );
                dr->set_camera( _pssm_rig->get_camera( 0 ) );
                dr->set_sort( -10000 );
        }
}

void BSPShaderGenerator::set_shader_quality( int quality )
{
        _shader_quality = quality;
        _gsg->mark_rehash_generated_shaders();
}

/**
 * Adds a new shader that can be used by Materials.
 * The ShaderSpec class will setup the correct permutations
 * for the shader based on the RenderState.
 */
void BSPShaderGenerator::add_shader( PT( ShaderSpec ) shader )
{
        _shaders[shader->get_name()] = shader;
	//shader->precache();
}

void BSPShaderGenerator::set_sun_light( const NodePath &np )
{
        if ( np.is_empty() )
        {
                if ( !_sunlight.is_empty() )
                        _sunlight.clear();
                _has_shadow_sunlight = false;
                _pssm_rig->reparent_to( NodePath() );
                return;
        }

        _sunlight = np;

        DirectionalLight *dlight = DCAST( DirectionalLight, _sunlight.node() );
        _sun_vector = -dlight->get_direction();

        _has_shadow_sunlight = true;

        _pssm_rig->reparent_to( _render );
}

void BSPShaderGenerator::update()
{
	_planar_reflections->update();

        if ( want_pssm )
        {
                if ( _sunlight.is_empty() & _has_shadow_sunlight )
                {
                        _has_shadow_sunlight = false;
                        _pssm_rig->reparent_to( NodePath() );
                }

                if ( _has_shadow_sunlight )
                        _pssm_rig->update( _camera.get_child( 0 ), _sun_vector );
        }

	if ( _fog )
	{
		_pta_fogdata[0] = _fog->get_color();

		_pta_fogdata[1][0] = _fog->get_exp_density();

		float start, stop;
		_fog->get_linear_range( start, stop );
		_pta_fogdata[1][1] = start;
		_pta_fogdata[1][2] = stop;

		_pta_fogdata[1][3] = 1.0f;//self->_fog->get_linear
	}
}

CPT( RenderAttrib ) apply_node_inputs( const RenderState *rs, CPT( RenderAttrib ) shattr )
{
        bool inputs_supplied = false;

        const AuxDataAttrib *ada;
        rs->get_attrib_def( ada );
        if ( ada->has_data() )
        {
                if ( ada->get_data()->is_exact_type( CNodeShaderInput::get_class_type() ) )
                {
                        CNodeShaderInput *bsp_node_input = DCAST( CNodeShaderInput, ada->get_data() );
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
                        inputs.push_back( ShaderInput( "envmapSampler", BSPShaderGenerator::get_identity_cubemap() ) );
                }

                shattr = DCAST( ShaderAttrib, shattr )->set_shader_inputs( inputs );
        }

        return shattr;
}

CPT( ShaderAttrib ) BSPShaderGenerator::synthesize_shader( const RenderState *rs,
        const GeomVertexAnimationSpec &anim )
{
	LightMutexHolder holder( synthesize_mutex );

        findmatshader_collector.start();

        // First figure out which shader to use.
        // UnlitNoMat by default, unless specified by a Material.
        std::string shader_name = DEFAULT_SHADER;

        const BSPMaterialAttrib *mattr;
        rs->get_attrib_def( mattr );
        const BSPMaterial *mat = mattr->get_material();
        if ( mattr->has_override_shader() )
        {
                // the attrib wants us to use this shader,
                // not the one referenced by the material
                shader_name = mattr->get_override_shader();
        }
        else if ( mat )
        {
                // no overrided shader, use the one specified on the material
                shader_name = mat->get_shader();
        }

        if ( _shaders.find( shader_name ) == _shaders.end() )
        {
                // We haven't heard about this shader, they must've not called
                // add_shader().

                std::ostringstream msg;
                msg << "Don't know about shader `" << shader_name << "`";
                if ( mat )
                {
                        msg << "referenced by material `"
                                << mat->get_file().get_fullpath() << "`\n";
                }
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

	PT( ShaderPermutations ) permutations = new ShaderPermutations;
        ShaderSpec *spec;

        spec = _shaders[shader_name];

	gen_perms_collector.start();
        spec->setup_permutations( *permutations, mat, rs, anim, this );
	gen_perms_collector.stop();

	complete_perms_collector.start();
	permutations->complete();
	complete_perms_collector.stop();

        if ( cache_shaders )
        {
		PStatTimer timer( lookup_collector );
#ifdef SHADER_PERMS_UNORDERED_MAP
                auto itr = spec->_generated_shaders.find( permutations );
                if ( itr != spec->_generated_shaders.end() )
                {
			CPT( ShaderAttrib ) shattr = itr->second;
#else
		int itr = spec->_generated_shaders.find( permutations );
		if ( itr != -1 )
		{
			CPT( ShaderAttrib ) shattr = spec->_generated_shaders.get_data( itr );
#endif
                        shattr = DCAST( ShaderAttrib, apply_node_inputs( rs, shattr ) );

                        return shattr;
                }
        }

	synthesize_collector.start();
	CPT( Shader ) shader = make_shader( spec, permutations );
	synthesize_collector.stop();

        nassertr( shader != nullptr, nullptr );

	make_attrib_collector.start();

        CPT( RenderAttrib ) shattr = ShaderAttrib::make( shader );

        // Add any inputs from the permutations.
        shattr = DCAST( ShaderAttrib, shattr )->set_shader_inputs( permutations->inputs );
        // Also any flags.
	size_t nflags = permutations->flag_indices.size();
	for ( size_t i = 0; i < nflags; i++ )
	{
		shattr = DCAST( ShaderAttrib, shattr )->set_flag( permutations->flag_indices[i], true );
	}

        CPT( ShaderAttrib ) attr = DCAST( ShaderAttrib, shattr );

        if ( cache_shaders )
                spec->_generated_shaders[permutations] = attr;

        shattr = apply_node_inputs( rs, shattr );
        attr = DCAST( ShaderAttrib, shattr );

	make_attrib_collector.stop();

        return attr;
}

void BSPShaderGenerator::set_identity_cubemap( Texture *tex )
{
	LightMutexHolder holder( cubemap_mutex );

        _identity_cubemap = tex;
	enable_srgb_read( tex, true );
}

Texture *BSPShaderGenerator::get_identity_cubemap()
{
	LightMutexHolder holder( cubemap_mutex );

        if ( !_identity_cubemap )
        {
                _identity_cubemap = TexturePool::load_cube_map( "materials/engine/defaultcubemap/defaultcubemap_#.jpg" );
		enable_srgb_read( _identity_cubemap, true );
        }

        return _identity_cubemap;
}

CPT( Shader ) BSPShaderGenerator::make_shader( const ShaderSpec *spec, const ShaderPermutations *perms )
{
	std::ostringstream vshader, gshader, fshader;

	// Slip the defines into the shader source.
	if ( spec->_vertex.has )
	{
		vshader << spec->_vertex.before_defines
			<< "\n" << perms->permutations
			<< spec->_vertex.after_defines;
	}
	if ( spec->_geom.has )
	{
		gshader << spec->_geom.before_defines
			<< "\n" << perms->permutations
			<< spec->_geom.after_defines;
	}
	if ( spec->_pixel.has )
	{
		fshader << spec->_pixel.before_defines
			<< "\n" << perms->permutations
			<< spec->_pixel.after_defines;
	}

	return Shader::make( Shader::SL_GLSL, vshader.str(), fshader.str(), gshader.str() );
}
