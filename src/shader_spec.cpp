/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file shader_spec.cpp
 * @author Brian Lach
 * @date November 02, 2018
 */

#include "shader_spec.h"
#include "shader_generator.h"
#include "bsp_material.h"
#include "bsploader.h"

#include <virtualFileSystem.h>
#include <colorBlendAttrib.h>

void ShaderSpec::ShaderSource::read( const Filename &file )
{
        VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();
        filename = file;
        if ( !filename.empty() )
        {
                if ( vfs->exists( filename ) )
                {
                        full_source = vfs->read_file( filename, true );

                        size_t end_of_first_line = full_source.find_first_of( '\n' );
                        before_defines = full_source.substr( 0, end_of_first_line );
                        after_defines = full_source.substr( end_of_first_line );

                        has = true;
                }
                else
                {
                        std::cout << "Couldn't find shader source file "
                                << filename.get_fullpath() << std::endl;
                }
        }
}

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
        Namable( name )
{
        read_shader_files( vert_file, pixel_file, geom_file );
}

void ShaderSpec::read_shader_files( const Filename &vert_file,
                                    const Filename &pixel_file,
                                    const Filename &geom_file )
{
        // load up and store the source of the shaders
        _vertex.read( vert_file );
        _pixel.read( pixel_file );
        _geom.read( geom_file );
}

ShaderConfig *ShaderSpec::get_shader_config( const BSPMaterial *mat )
{
        int idx = _config_cache.find( mat );
        if ( idx == -1 )
        {
                PT( ShaderConfig ) conf = make_new_config();
                conf->parse_from_material_keyvalues( mat );
                _config_cache[mat] = conf;
                return conf;
        }

        return _config_cache.get_data( idx );
}

void ShaderSpec::setup_permutations( ShaderPermutations &result,
	const BSPMaterial *mat,
	const RenderState *state,
	const GeomVertexAnimationSpec &anim,
	BSPShaderGenerator *generator )
{
        result.add_permutation( "SHADER_QUALITY", generator->get_shader_quality() );

	const LightRampAttrib *lra;
	state->get_attrib_def( lra );
	if ( lra->get_mode() != LightRampAttrib::LRT_default )
	{
		result.add_permutation( "HDR" );
		result.add_input( ShaderInput( "_exposureAdjustment", generator->get_exposure_adjustment() ) );
	}

	const ColorBlendAttrib *cba;
	state->get_attrib_def( cba );
	if ( cba->get_mode() != ColorBlendAttrib::M_none )
	{
		if ( cba->get_mode() == ColorBlendAttrib::M_add &&
		     cba->get_operand_a() == ColorBlendAttrib::O_one &&
		     cba->get_operand_b() == ColorBlendAttrib::O_one )
		{
			result.add_permutation( "BLEND_ADDITIVE" );
		}
		else if ( cba->get_mode() == ColorBlendAttrib::M_add &&
			  cba->get_operand_a() == ColorBlendAttrib::O_fbuffer_color &&
			  cba->get_operand_b() == ColorBlendAttrib::O_incoming_color )
		{
			result.add_permutation( "BLEND_MODULATE" );
		}
	}
}

#include <fogAttrib.h>

bool ShaderSpec::add_fog( const RenderState *rs, ShaderPermutations &perms,
	BSPShaderGenerator *generator )
{
	const FogAttrib *fa;
	rs->get_attrib_def( fa );
        // Check for fog.
        if ( generator->get_fog() && !fa->is_off() )
        {
                perms.add_permutation( "FOG", (int)generator->get_fog()->get_mode() );
		perms.add_input( ShaderInput( "fogData", generator->get_fog_data() ) );
		return true;
        }

	return false;
}

#include <colorAttrib.h>

void ShaderSpec::add_color( const RenderState *rs, ShaderPermutations &perms )
{
}

#include <auxBitplaneAttrib.h>

bool ShaderSpec::add_aux_bits( const RenderState *rs, ShaderPermutations &perms )
{
	bool has_them = false;

	const AuxBitplaneAttrib *aba;
	rs->get_attrib_def( aba );
	if ( ( aba->get_outputs() & AUXBITS_NORMAL ) != 0 )
	{
		perms.add_permutation( "NEED_AUX_NORMAL" );
		has_them = true;
	}
	if ( ( aba->get_outputs() & AUXBITS_ARME ) != 0 )
	{
		perms.add_permutation( "NEED_AUX_ARME" );
		has_them = true;
	}

	return has_them;
}

#include "pssmCameraRig.h"

bool ShaderSpec::add_csm( const RenderState *rs, ShaderPermutations &result, BSPShaderGenerator *generator )
{
        if ( generator->has_shadow_sunlight() )
        {
                result.add_permutation( "HAS_SHADOW_SUNLIGHT" );
		result.add_permutation( "PSSM_SPLITS", pssm_splits.get_string_value() );
		result.add_permutation( "DEPTH_BIAS", depth_bias.get_string_value() );
		result.add_permutation( "NORMAL_OFFSET_SCALE", normal_offset_scale.get_string_value() );

                float xel_size = 1.0 / pssm_size.get_value();

                result.add_permutation( "SHADOW_BLUR", xel_size * softness_factor.get_value() );
                result.add_permutation( "SHADOW_TEXEL_SIZE", xel_size );

                if ( normal_offset_uv_space.get_value() )
                        result.add_permutation( "NORMAL_OFFSET_UV_SPACE" );

                result.add_input( ShaderInput( "pssmSplitSampler", generator->get_pssm_array_texture() ) );
                result.add_input( ShaderInput( "pssmMVPs", generator->get_pssm_rig()->get_mvp_array() ) );

                BSPLoader *loader = BSPLoader::get_global_ptr();
                result.add_input( ShaderInput( "sunVector", generator->get_pssm_rig()->get_sun_vector() ) );
                if ( loader->get_ambient_probe_mgr()->get_sunlight() )
                        result.add_input( ShaderInput( "sunColor", loader->get_ambient_probe_mgr()->get_sunlight()->color ) );
                else
                        result.add_input( ShaderInput( "sunColor", LVector3( 1, 1, 1 ) ) );

                return true;
        }

        return false;
}

#include <clipPlaneAttrib.h>

bool ShaderSpec::add_clip_planes( const RenderState *rs, ShaderPermutations &perms )
{
        // Check for clip planes.
        const ClipPlaneAttrib *clip_plane;
        rs->get_attrib_def( clip_plane );

        perms.add_permutation( "NUM_CLIP_PLANES", clip_plane->get_num_on_planes() );

        return clip_plane->get_num_on_planes() > 0;
}

void ShaderSpec::add_hw_skinning( const GeomVertexAnimationSpec &anim, ShaderPermutations &perms )
{
        // Hardware skinning?
        if ( anim.get_animation_type() == GeomEnums::AT_hardware &&
                anim.get_num_transforms() > 0 )
        {
		perms.add_permutation( "HARDWARE_SKINNING" );
                int num_transforms;
                if ( anim.get_indexed_transforms() )
                {
                        num_transforms = 120;
                }
                else
                {
                        num_transforms = anim.get_num_transforms();
                }
                perms.add_permutation( "NUM_TRANSFORMS", num_transforms );

                if ( anim.get_indexed_transforms() )
                {
			perms.add_permutation( "INDEXED_TRANSFORMS" );
                }
        }
}

#include <alphaTestAttrib.h>

bool ShaderSpec::add_alpha_test( const RenderState *rs, ShaderPermutations &perms )
{
	const AlphaTestAttrib *alpha_test;
	rs->get_attrib_def( alpha_test );
	if ( alpha_test->get_mode() != RenderAttrib::M_none &&
		alpha_test->get_mode() != RenderAttrib::M_always )
	{
		// Subsume the alpha test in our shader.
		perms.add_permutation( "ALPHA_TEST", alpha_test->get_mode() );
		perms.add_permutation( "ALPHA_TEST_REF", alpha_test->get_reference_alpha() );

		perms.add_flag( ShaderAttrib::F_subsume_alpha_test );

		return true;
	}

	return false;
}

void ShaderSpec::r_precache( ShaderPrecacheCombos &combos )
{
	size_t n = combos.combos.size();

	int *indices = new int[n];
	memset( indices, 0, sizeof( int ) * n );

	int permutations = 0;

	while ( 1 )
	{
		ShaderPermutations perms;
		perms.local_object();
		for ( int i = 0; i < n; i++ )
		{
			if ( combos.combos[i].is_bool && combos.combos[i].min_val + indices[n] == 0 )
				continue;
			perms.add_permutation( combos.combos[i].combo_name,
					       combos.combos[i].min_val + indices[n] );
		}

		BSPShaderGenerator::make_shader( this, &perms );
		permutations++;
		std::cout << "\tCompiled " << permutations << " permutations\n";

		int next = n - 1;
		while ( next >= 0 && indices[next] + 1 >= ( combos.combos[next].max_val - combos.combos[next].min_val ) + 1 )
		{
			next--;
		}

		if ( next < 0 )
		{
			delete[] indices;
			return;
		}
			

		indices[next]++;

		for (int i = next + 1; i < n; i++ )
		{
			indices[i] = 0;
		}
	}

}

void ShaderSpec::precache()
{
	ShaderPrecacheCombos combos;
	add_precache_combos( combos );

	int total_combos = 0;
	for ( size_t i = 0; i < combos.combos.size(); i++ )
	{
		int possibilities = ( combos.combos[i].max_val - combos.combos[i].min_val ) + 1;
		if ( total_combos == 0 )
			total_combos += possibilities;
		else
			total_combos *= possibilities;
	}

	std::cout << "Precaching " << total_combos << " static combos for shader " << get_name() << std::endl;

	r_precache( combos );
}

void ShaderSpec::add_precache_combos( ShaderPrecacheCombos &combos )
{
	combos.add( "SHADER_QUALITY", 2, 2 );
}

//=================================================================================================

#include <pnmImage.h>

void enable_srgb_read( Texture *tex, bool enable )
{
	if ( !tex )
		return;

	if ( enable )
	{
		switch ( tex->get_num_components() )
		{
		case 4:
			tex->set_format( Texture::F_srgb_alpha );
			break;
		case 3:
			tex->set_format( Texture::F_srgb );
			break;
		//case 2:
		//	tex->set_format( Texture::F_sluminance_alpha );
		//	break;
		//case 1:
		//	tex->set_format( Texture::F_sluminance );
		//	break;
		}
	}
	else
	{
		switch ( tex->get_num_components() )
		{
		case 4:
			tex->set_format( Texture::F_rgba );
			break;
		case 3:
			tex->set_format( Texture::F_rgb );
			break;
		case 2:
			tex->set_format( Texture::F_luminance_alpha );
			break;
		case 1:
			tex->set_format( Texture::F_luminance );
			break;
		}
	}
}
