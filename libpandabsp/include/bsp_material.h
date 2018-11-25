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

#include <material.h>
#include <shaderInput.h>
#include <pmap.h>
#include <textureStage.h>

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
        static TextureStage *get_bumped_lightmap( int n );
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

class Materials
{
PUBLISHED:
        static Material *get( Material *mat );

private:
        static LightMutex _lock;

        // We store a map of CPT(Material) to PT(Material).  These are two
        // equivalent structures, but different pointers.  The first pointer never
        // leaves this class.  If the second pointer changes value, we'll notice it
        // and return a new one.
        typedef pmap< CPT( Material ), PT( Material ), indirect_compare_to<const Material *> > matmap_t;
        static matmap_t _materials;
};

#define DEFAULT_SHADER "VertexLitGeneric"

class BSPMaterial : public Material
{
PUBLISHED:
        INLINE explicit BSPMaterial( const std::string &name = "" ) :
                Material( name ),
                _shader_name( DEFAULT_SHADER )
        {
        }

        INLINE BSPMaterial( const BSPMaterial &copy ) :
                Material( copy ),
                _shader_name( copy._shader_name ),
                _shader_inputs( copy._shader_inputs )
        {
        }

        INLINE void operator = ( const BSPMaterial &copy )
        {
                Material::operator = ( copy );
                _shader_name = copy._shader_name;
                _shader_inputs = copy._shader_inputs;
        }


        void set_shader( const std::string &shader_name );
        INLINE std::string get_shader() const
        {
                return _shader_name;
        }

        void set_shader_input( const ShaderInput &input );
        INLINE const pvector<ShaderInput> &get_shader_inputs() const
        {
                return _shader_inputs;
        }

        virtual int compare_to_impl(const Material *other) const;
        virtual size_t get_hash_impl() const;

private:
        std::string _shader_name;
        pvector<ShaderInput> _shader_inputs;

public:
        static void register_with_read_factory();
        virtual void write_datagram( BamWriter *manager, Datagram &me );

protected:
        static TypedWritable *make_BSPMaterial( const FactoryParams &params );
        void fillin( DatagramIterator &scan, BamReader *manager );

public:
        static TypeHandle get_class_type()
        {
                return _type_handle;
        }
        static void init_type()
        {
                Material::init_type();
                register_type( _type_handle, "BSPMaterial",
                               Material::get_class_type() );
        }
        virtual TypeHandle get_type() const
        {
                return get_class_type();
        }
        virtual TypeHandle force_init_type() { init_type(); return get_class_type(); }

private:
        static TypeHandle _type_handle;
};

#endif // BSP_MATERIAL_H