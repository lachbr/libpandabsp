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
        _geom_file( geom_file ),
        _has_vertex(false ),
        _has_pixel(false),
        _has_geom(false)
{
        read_shader_files();
}

void ShaderSpec::read_shader_files()
{
        VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();
        // load up and store the source of the shaders
        if ( !_vertex_file.empty() )
        {
                if ( vfs->exists( _vertex_file ) )
                {
                        _vertex_source = vfs->read_file( _vertex_file, true );
                        _has_vertex = true;
                }
                else
                {
                        std::cout << "Can't find vertex shader file " << _vertex_file << std::endl;
                }
        }
        if ( !_pixel_file.empty() )
        {
                if ( vfs->exists( _pixel_file ) )
                {
                        _pixel_source = vfs->read_file( _pixel_file, true );
                        _has_pixel = true;
                }
                else
                {
                        std::cout << "Can't find pixel shader file " << _pixel_file << std::endl;
                }
        }
        if ( !_geom_file.empty() )
        {
                if ( vfs->exists( _geom_file ) )
                {
                        _geom_source = vfs->read_file( _geom_file, true );
                        _has_geom = true;
                }
                else
                {
                        std::cout << "Can't find geometry shader file " << _geom_file << std::endl;
                }
        }        
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