/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file bsp_material.cpp
 * @author Brian Lach
 * @date November 02, 2018
 */

#include "bsp_material.h"

//====================================================================//

TextureStages::tspool_t TextureStages::_stage_pool;

/**
 * Returns the texture stage with the given name,
 * or a new one if it doesn't already exists.
 */
TextureStage *TextureStages::get( const std::string &name )
{
        auto itr = _stage_pool.find( name );
        if ( itr != _stage_pool.end() )
        {
                return itr->second;
        }

        // Texture stage with this name doesn't exist,
        // create it.

        PT( TextureStage ) stage = new TextureStage( name );
        _stage_pool[name] = stage;

        return stage;
}

TextureStage *TextureStages::get( const std::string &name, const std::string &uv_name )
{
        auto itr = _stage_pool.find( name );
        if ( itr != _stage_pool.end() )
        {
                return itr->second;
        }

        // Texture stage with this name doesn't exist,
        // create it. Also, assign the provided UV name.

        PT( TextureStage ) stage = new TextureStage( name );
        stage->set_texcoord_name( uv_name );
        _stage_pool[name] = stage;

        return stage;
}

TextureStage *TextureStages::get_basetexture()
{
        return get( "basetexture", "basetexture" );
}

TextureStage *TextureStages::get_lightmap()
{
        return get( "lightmap", "lightmap" );
}

TextureStage *TextureStages::get_bumped_lightmap()
{
        return get( "lightmap_bumped", "lightmap" );
}

TextureStage *TextureStages::get_spheremap()
{
        return get( "spheremap" );
}

TextureStage *TextureStages::get_cubemap()
{
        return get( "cubemap_tex" );
}

TextureStage *TextureStages::get_heightmap()
{
        return get( "heightmap" );
}

TextureStage *TextureStages::get_normalmap()
{
        return get( "normalmap" );
}

TextureStage *TextureStages::get_glossmap()
{
        return get( "glossmap" );
}

TextureStage *TextureStages::get_glowmap()
{
        return get( "glowmap" );
}

//====================================================================//
//====================================================================//

#include "viftokenizer.h"
#include "vifparser.h"
#include <virtualFileSystem.h>

NotifyCategoryDef( bspmaterial, "" );

TypeHandle BSPMaterial::_type_handle;

BSPMaterial::materialcache_t BSPMaterial::_material_cache;

const BSPMaterial *BSPMaterial::get_from_file( const Filename &file )
{
        int idx = _material_cache.find( file );
        if ( idx != -1 )
        {
                // We've already loaded this material file.
                return _material_cache.get_data( idx );
        }
        
        VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();
        if ( !vfs->exists( file ) )
        {
                bspmaterial_cat.error()
                        << "Could not find material file " << file.get_fullpath() << "\n";
                return nullptr;
        }

        bspmaterial_cat.info()
                << "Loading material " << file.get_fullpath() << "\n";

        PT( BSPMaterial ) mat = new BSPMaterial;
        mat->_file = file;

        string mat_data = vfs->read_file( file, true );
        TokenVec toks = tokenizer( mat_data );
        Parser p( toks );

        Object obj = p._base_objects[0];
        mat->set_shader( obj.name ); // ->VertexLitGeneric<- {...}
        pvector<Property> props = p.get_properties( obj );
        for ( size_t i = 0; i < props.size(); i++ )
        {
                const Property &prop = props[i];
                mat->set_keyvalue( prop.name, prop.value ); // "$basetexture"   "phase_3/maps/desat_shirt_1.jpg"
        }

        // Figure out these values and store
        // for easy access elsewhere.
        mat->_has_env_cubemap = ( mat->has_keyvalue( "$envmap" ) && mat->get_keyvalue( "$envmap" ) == "env_cubemap" );
        if ( mat->has_keyvalue( "$surfaceprop" ) )
                mat->_surfaceprop = mat->get_keyvalue( "$surfaceprop" );
        if ( mat->has_keyvalue( "$contents" ) )
                mat->_contents = mat->get_keyvalue( "$contents" );
        mat->_has_transparency = ( mat->has_keyvalue( "$translucent" ) && atoi( mat->get_keyvalue( "$translucent" ).c_str() ) == 1 ) ||
                ( mat->has_keyvalue( "$alpha" ) && atof( mat->get_keyvalue( "$alpha" ).c_str() ) < 1.0 );

        _material_cache[file] = mat;

        return mat;
}

//====================================================================//

TypeHandle BSPMaterialAttrib::_type_handle;
int BSPMaterialAttrib::_attrib_slot;

CPT( RenderAttrib ) BSPMaterialAttrib::make( const BSPMaterial *mat )
{
        PT( BSPMaterialAttrib ) bma = new BSPMaterialAttrib;
        bma->_mat = mat;
        bma->_has_override_shader = false;
        return return_new( bma );
}

/**
 * Creates a new BSPMaterialAttrib that says the shader name on the attrib
 * should override any other material's shader it composes with, but keep the keyvalues.
 *
 * This is useful for something like a shadow render pass, where all objects are rendered
 * using the shadow pass shader, but need access to the $basetexture of each material
 * for transparency and possibly other effects.
 *
 * If this didn't exist, as the RenderState composes, each BSPMaterialAttrib would
 * completely override each other, voiding the shadow render shader.
 */
CPT( RenderAttrib ) BSPMaterialAttrib::make_override_shader( const BSPMaterial *mat )
{
        PT( BSPMaterialAttrib ) bma = new BSPMaterialAttrib;
        bma->_mat = mat;
        bma->_has_override_shader = true;
        bma->_override_shader = mat->get_shader();
        return return_new( bma );
}

CPT( RenderAttrib ) BSPMaterialAttrib::make_default()
{
        PT( BSPMaterial ) mat = new BSPMaterial;
        PT( BSPMaterialAttrib ) bma = new BSPMaterialAttrib;
        bma->_mat = mat;
        bma->_has_override_shader = false;
        return return_new( bma );
}

CPT( RenderAttrib ) BSPMaterialAttrib::compose_impl( const RenderAttrib *other ) const
{
        const BSPMaterialAttrib *bma = (const BSPMaterialAttrib *)other;

        if ( _has_override_shader )
        {
                // We're going to override the other material's shader,
                // but keep their keyvalues.
                BSPMaterialAttrib *nbma = new BSPMaterialAttrib;
                nbma->_mat = bma->_mat;
                nbma->_has_override_shader = true;
                nbma->_override_shader = _override_shader;
                return return_new( nbma );
        }

        return other;
}

CPT( RenderAttrib ) BSPMaterialAttrib::invert_compose_impl( const RenderAttrib *other ) const
{
        const BSPMaterialAttrib *bma = (const BSPMaterialAttrib *)other;

        if ( bma->_has_override_shader )
        {
                // The other material is going to override our shader.
                BSPMaterialAttrib *nbma = new BSPMaterialAttrib;
                nbma->_mat = bma->_mat;
                nbma->_has_override_shader = true;
                nbma->_override_shader = bma->_override_shader;
                return return_new( nbma );
        }

        return other;
}

/**
 * BSPMaterials are compared solely by their source filename.
 * We could also compare all of the keyvalues, but whatever.
 * You shouldn't really be creating BSPMaterials on the fly,
 * they should always be in a file.
 */
int BSPMaterialAttrib::compare_to_impl( const RenderAttrib *other ) const
{
        const BSPMaterialAttrib *bma = (const BSPMaterialAttrib *)other;

        if ( _mat != bma->_mat )
        {
                return _mat < bma->_mat ? -1 : 1;
        }
        if ( _has_override_shader != bma->_has_override_shader )
        {
                return (int)_has_override_shader < (int)bma->_has_override_shader ? -1 : 1;
        }

        return _override_shader.compare( bma->_override_shader );
}

size_t BSPMaterialAttrib::get_hash_impl() const
{
        size_t hash = 0;
        hash = pointer_hash::add_hash( hash, _mat );
        hash = int_hash::add_hash( hash, _has_override_shader );
        hash = string_hash::add_hash( hash, _override_shader );
        return hash;
}

void BSPMaterialAttrib::register_with_read_factory()
{
        BamReader::get_factory()->register_factory( get_class_type(), make_from_bam );
}

void BSPMaterialAttrib::write_datagram( BamWriter *manager, Datagram &dg )
{
        RenderAttrib::write_datagram( manager, dg );
        dg.add_string( _mat->get_file().get_fullpath() );
}

TypedWritable *BSPMaterialAttrib::make_from_bam( const FactoryParams &params )
{
        BSPMaterialAttrib *bma = new BSPMaterialAttrib;
        DatagramIterator scan;
        BamReader *manager;

        parse_params( params, scan, manager );
        bma->fillin( scan, manager );

        return bma;
}

void BSPMaterialAttrib::fillin( DatagramIterator &scan, BamReader *manager )
{
        RenderAttrib::fillin( scan, manager );

        _mat = BSPMaterial::get_from_file( scan.get_string() );
}