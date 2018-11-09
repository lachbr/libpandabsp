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

TextureStage *TextureStages::get_bumped_lightmap( int n )
{
        std::stringstream ss;
        ss << "lightmap" << n;
        return get( ss.str(), "lightmap" );
}

TextureStage *TextureStages::get_spheremap()
{
        return get( "spheremap" );
}

TextureStage *TextureStages::get_cubemap()
{
        return get( "cubemap" );
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

Materials::matmap_t Materials::_materials;
LightMutex Materials::_lock;

Material *Materials::get( Material *mat )
{
        LightMutexHolder holder( _lock );
        CPT( Material ) cpttemp = mat;

        auto itr = _materials.find( cpttemp );
        if ( itr == _materials.end() )
        {
                PT( Material ) newmat = nullptr;
                if ( mat->is_exact_type( Material::get_class_type() ) )
                {
                        newmat = new Material( *mat );
                }
                else if ( mat->is_exact_type( BSPMaterial::get_class_type() ) )
                {
                        newmat = new BSPMaterial( *DCAST( BSPMaterial, mat ) );
                }

                nassertr( newmat != nullptr, nullptr );

                itr = _materials.insert( matmap_t::value_type( newmat, mat ) ).first;
        }
#if 0
        else
        {
                if ( *( *itr ).first != *( *itr ).second )
                {
                        // The pointer no longer matches its original value.  Save a new one.
                        ( *itr ).second = mat;
                }
        }
#endif

        return ( *itr ).second;
}

//====================================================================//

TypeHandle BSPMaterial::_type_handle;

/**
 * Sets the name of the shader that this material will invoke.
 * Ex: "VertexLitGeneric", "LightmappedGeneric", "Water"
 */
void BSPMaterial::set_shader( const std::string &shader_name )
{
        _shader_name = shader_name;
}

/**
 * Adds a new input to the shader that runs for this material.
 */
void BSPMaterial::set_shader_input( const ShaderInput &input )
{
        _shader_inputs.push_back( input );
}

size_t BSPMaterial::get_hash_impl() const
{
        size_t hash = Material::get_hash_impl();

        hash = string_hash::add_hash( hash, _shader_name );
        for ( size_t i = 0; i < _shader_inputs.size(); i++ )
        {
                hash = _shader_inputs[i].add_hash( hash );
        }

        return hash;
}

int BSPMaterial::compare_to_impl( const Material *mother ) const
{
        const BSPMaterial *other = (const BSPMaterial *)mother;
        int mat_result = Material::compare_to_impl( mother );
        if ( mat_result != 0 )
        {
                return mat_result;
        }

        if ( _shader_name != other->_shader_name )
        {
                return _shader_name.compare( other->_shader_name );
        }

        if ( _shader_inputs.size() != other->_shader_inputs.size() )
        {
                return _shader_inputs.size() < other->_shader_inputs.size() ? -1 : 1;
        }
        for ( size_t i = 0; i < _shader_inputs.size(); i++ )
        {
                if ( _shader_inputs[i] != other->_shader_inputs[i] )
                {
                        return _shader_inputs[i] < other->_shader_inputs[i] ? -1 : 1;
                }
        }

        // equal
        return 0;
}

//=====================================================================
//======================== BAM STUFF ==================================

void BSPMaterial::register_with_read_factory()
{
        BamReader::get_factory()->register_factory( get_class_type(), make_BSPMaterial );
}

void BSPMaterial::write_datagram( BamWriter *manager, Datagram &me )
{
        Material::write_datagram( manager, me );
        me.add_string( _shader_name );
}

TypedWritable *BSPMaterial::make_BSPMaterial( const FactoryParams &params )
{
        BSPMaterial *mat = new BSPMaterial;
        DatagramIterator scan;
        BamReader *manager;
        parse_params( params, scan, manager );
        mat->fillin( scan, manager );
        return mat;
}

void BSPMaterial::fillin( DatagramIterator &scan, BamReader *manager )
{
        Material::fillin( scan, manager );
        set_shader( scan.get_string() );
}