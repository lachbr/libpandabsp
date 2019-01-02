/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file shader_spec.cpp
 * @author Brian Lach
 * @date November 02, 2018
 */

#include "shader_spec.h"
#include "shader_generator.h"
#include "bsp_material.h"

#include <virtualFileSystem.h>

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

ShaderPermutations ShaderSpec::setup_permutations( const BSPMaterial *mat,
                                                   const RenderState *state,
                                                   const GeomVertexAnimationSpec &anim, 
                                                   PSSMShaderGenerator *generator )
{
        ShaderPermutations result;
        return result;
}

#include <fogAttrib.h>

void ShaderSpec::add_fog( const RenderState *rs, ShaderPermutations &perms )
{
        // Check for fog.
        const FogAttrib *fog;
        if ( rs->get_attrib( fog ) && !fog->is_off() )
        {
                std::stringstream fss;
                fss << (int)fog->get_fog()->get_mode();
                perms.add_permutation( "FOG", fss.str() );
        }
}

//=================================================================================================