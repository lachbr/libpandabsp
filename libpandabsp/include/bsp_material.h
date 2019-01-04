/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file bsp_material.h
 * @author Brian Lach
 * @date November 02, 2018
 */

#ifndef BSP_MATERIAL_H
#define BSP_MATERIAL_H

#include "config_bsp.h"

#include <material.h>
#include <shaderInput.h>
#include <pmap.h>
#include <textureStage.h>
#include <renderAttrib.h>

/**
 * This simple interface maintains a single TextureStage object for each unique name.
 * It avoids the creation of duplicate TextureStages with the same name, which
 * reduces texture swapping and draw call overhead.
 *
 * If using our shader system, you should always use this interface to get TextureStages.
 * You are not required to change any properties on the returned TextureStage, as the shader
 * specification will know what to do with the TextureStage from the name.
 * 
 * For example, you do not need to call TextureStage::set_mode() or NodePath::set_tex_gen().
 * If you apply a texture to a node with the get_normalmap() stage, the shader specification
 * will know that the texture you supplied is to be treated as a normal map.
 */
class TextureStages
{
PUBLISHED:
        static TextureStage *get( const std::string &name );
        static TextureStage *get( const std::string &name, const std::string &uv_name );

        static TextureStage *get_basetexture();
        static TextureStage *get_lightmap();
        static TextureStage *get_bumped_lightmap();
        static TextureStage *get_spheremap();
        static TextureStage *get_cubemap();
        static TextureStage *get_normalmap();
        static TextureStage *get_heightmap();
        static TextureStage *get_glossmap();
        static TextureStage *get_glowmap();

private:
        typedef pmap<std::string, PT( TextureStage )> tspool_t;
        static tspool_t _stage_pool;
};

#define DEFAULT_SHADER "VertexLitGeneric"

NotifyCategoryDeclNoExport(bspmaterial);

#ifdef CPPPARSER
class BSPMaterial : public TypedReferenceCount
#else
class EXPCL_PANDABSP BSPMaterial : public TypedReferenceCount
#endif
{
PUBLISHED:
        INLINE explicit BSPMaterial( const std::string &name = DEFAULT_SHADER ) :
                TypedReferenceCount(),
                _has_env_cubemap( false ),
                _cached_env_cubemap( false ),
                _shader_name( name ),
                _surfaceprop( "default" ),
                _contents( "solid" )
        {
        }

        INLINE BSPMaterial( const BSPMaterial &copy ) :
                TypedReferenceCount( copy ),
                _shader_name( copy._shader_name ),
                _shader_keyvalues( copy._shader_keyvalues ),
                _file( copy._file ),
                _has_env_cubemap( copy._has_env_cubemap ),
                _cached_env_cubemap( copy._cached_env_cubemap ),
                _surfaceprop( copy._surfaceprop ),
                _contents( copy._contents )
        {
        }

        INLINE void operator = ( const BSPMaterial &copy )
        {
               TypedReferenceCount::operator = ( copy );
                _shader_name = copy._shader_name;
                _shader_keyvalues = copy._shader_keyvalues;
                _file = copy._file;
                _has_env_cubemap = copy._has_env_cubemap;
                _cached_env_cubemap = copy._cached_env_cubemap;
                _surfaceprop = copy._surfaceprop;
                _contents = copy._contents;
        }

        INLINE void set_keyvalue( const std::string &key, const std::string &value )
        {
                _shader_keyvalues[key] = value;
        }
        INLINE std::string get_keyvalue( const std::string &key ) const
        {
                return _shader_keyvalues.get_data( _shader_keyvalues.find( key ) );
        }

        INLINE void set_shader( const std::string &shader_name )
        {
                _shader_name = shader_name;
        }
        INLINE std::string get_shader() const
        {
                return _shader_name;
        }

        INLINE Filename get_file() const
        {
                return _file;
        }

        INLINE bool has_keyvalue( const std::string &key ) const
        {
                return _shader_keyvalues.find( key ) != -1;
        }

        INLINE bool has_env_cubemap() const
        {
                return _has_env_cubemap;
        }

        INLINE std::string get_surface_prop() const
        {
                return _surfaceprop;
        }

        INLINE std::string get_contents() const
        {
                return _contents;
        }

        static const BSPMaterial *get_from_file( const Filename &file );

private:
        Filename _file;
        std::string _shader_name;
        bool _has_env_cubemap;
        bool _cached_env_cubemap;
        std::string _surfaceprop;
        std::string _contents;
        SimpleHashMap<std::string, std::string, string_hash> _shader_keyvalues;

        typedef SimpleHashMap<std::string, CPT( BSPMaterial ), string_hash> materialcache_t;
        static materialcache_t _material_cache;

public:
        static TypeHandle get_class_type()
        {
                return _type_handle;
        }
        static void init_type()
        {
                TypedReferenceCount::init_type();
                register_type( _type_handle, "BSPMaterial",
                               TypedReferenceCount::get_class_type() );
        }
        virtual TypeHandle get_type() const
        {
                return get_class_type();
        }
        virtual TypeHandle force_init_type() { init_type(); return get_class_type(); }

private:
        static TypeHandle _type_handle;
};

#ifdef CPPPARSER
class BSPMaterialAttrib : public RenderAttrib
#else
class EXPCL_PANDABSP BSPMaterialAttrib : public RenderAttrib
#endif
{
private:
        INLINE BSPMaterialAttrib() :
                RenderAttrib(),
                _mat( nullptr )
        {
        }

PUBLISHED:

        static CPT( RenderAttrib ) make( const BSPMaterial *mat );
        static CPT( RenderAttrib ) make_default();

        INLINE const BSPMaterial *get_material() const
        {
                return _mat;
        }

private:
        const BSPMaterial *_mat;

public:
        static void register_with_read_factory();
        virtual void write_datagram( BamWriter *manager, Datagram &dg );

protected:
        virtual int compare_to_impl( const RenderAttrib *other ) const;
        virtual size_t get_hash_impl() const;
        static TypedWritable *make_from_bam( const FactoryParams &params );
        void fillin( DatagramIterator &scan, BamReader *manager );

PUBLISHED:
        static int get_class_slot()
        {
                return _attrib_slot;
        }
        virtual int get_slot() const
        {
                return get_class_slot();
        }
        MAKE_PROPERTY( class_slot, get_class_slot );

public:
        static TypeHandle get_class_type()
        {
                return _type_handle;
        }
        static void init_type()
        {
                RenderAttrib::init_type();
                register_type( _type_handle, "BSPMaterialAttrib",
                               RenderAttrib::get_class_type() );
                _attrib_slot = register_slot( _type_handle, 100, new BSPMaterialAttrib );
        }
        virtual TypeHandle get_type() const
        {
                return get_class_type();
        }
        virtual TypeHandle force_init_type()
        {
                init_type(); return get_class_type();
        }

private:
        static TypeHandle _type_handle;
        static int _attrib_slot;
};

#endif // BSP_MATERIAL_H