/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file shader_spec.h
 * @author Brian Lach
 * @date November 02, 2018
 */

#ifndef SHADER_SPEC_H
#define SHADER_SPEC_H

#include <referenceCount.h>
#include <namable.h>
#include <pmap.h>
#include <shaderAttrib.h>
#include <geomVertexAnimationSpec.h>

class RenderState;
class PSSMShaderGenerator;
class BSPMaterial;

class ShaderConfig : public ReferenceCount
{
public:
        virtual void parse_from_material_keyvalues( const BSPMaterial *mat ) = 0;

        int flags;
};

class ShaderPermutations
{
public:
        pmap<std::string, std::string> permutations;

        vector_int flags;

        struct Input
        {
                bool important;
                ShaderInput input;

                bool operator == ( const Input &other )
                {
                        return ( input == other.input );
                }

                bool operator <( const Input &other )
                {
                        return input < other.input;
                }
        };

        pmap<std::string, Input> inputs;

PUBLISHED:

        void add_permutation( const std::string &key, const std::string &value = "1" )
        {
                permutations[key] = value;
        }

        void add_input( const ShaderInput &inp, bool important = true )
        {
                Input input;
                input.input = inp;
                input.important = important;
                inputs[inp.get_name()->get_name()] = input;
        }

        void add_flag( int flag )
        {
                flags.push_back( flag );
        }

        bool operator ==( const ShaderPermutations &other ) const
        {
                if ( permutations.size() != other.permutations.size() )
                {
                        return false;
                }
                std::cout << "Checking if permutations equal" << std::endl;
                auto itr = permutations.begin();
                auto itr2 = other.permutations.begin();
                for ( ; itr != permutations.end(); itr++, itr2++ )
                {
                        if ( itr->first != itr2->first )
                        {
                                return false;
                        }
                        if ( itr->second != itr2->second )
                        {
                                return false;
                        }
                }
                std::cout << "Permutations are equal" << std::endl;
                //if ( inputs != other.inputs )
                //{
                //        return false;
                //}
                return flags == other.flags;
        }

        bool operator !=( const ShaderPermutations &other ) const
        {
                return !operator == ( other );
        }

        bool operator < ( const ShaderPermutations &other ) const
        {
                if ( permutations.size() != other.permutations.size() )
                {
                        return permutations.size() < other.permutations.size();
                }
                auto itr = permutations.begin();
                auto itr2 = other.permutations.begin();
                for ( ; itr != permutations.end(); itr++, itr2++ )
                {
                        if ( itr->first != itr2->first )
                        {
                                return itr->first < itr2->first;
                        }
                        if ( itr->second != itr2->second )
                        {
                                return itr->second < itr2->second;
                        }
                }
                if ( inputs.size() != other.inputs.size() )
                {
                        return inputs.size() < other.inputs.size();
                }

                auto iitr = inputs.begin();
                auto iitr2 = other.inputs.begin();
                for ( ; iitr != inputs.end(); iitr++, iitr2++ )
                {
                        const Input &a = iitr->second;
                        const Input &b = iitr2->second;
                        if ( !a.important )
                        {
                                continue;
                        }
                        if ( a.input != b.input )
                        {
                                return a.input < b.input;
                        }
                }
                if ( flags.size() != other.flags.size() )
                {
                        return flags.size() < other.flags.size();
                }
                for ( size_t i = 0; i < flags.size(); i++ )
                {
                        if ( flags[i] != other.flags[i] )
                        {
                                return flags[i] < other.flags[i];
                        }
                }

                return flags < other.flags;
        }
};

/**
 * Serves to setup the permutations for a specific shader
 * when setting up a shader for a specific RenderState.
 */
class ShaderSpec : public ReferenceCount, public Namable
{
PUBLISHED:
        ShaderSpec( const std::string &name, const Filename &vert_file,
                    const Filename &pixel_file, const Filename &geom_file = "" );

        void read_shader_files();

public:
        virtual ShaderPermutations setup_permutations( const BSPMaterial *mat, const RenderState *state,
                                                 const GeomVertexAnimationSpec &anim, PSSMShaderGenerator *generator );
        

        ShaderConfig *get_shader_config( const BSPMaterial *mat );
        virtual PT( ShaderConfig ) make_new_config() = 0;

        static void add_fog( const RenderState *rs, ShaderPermutations &perms );

        typedef SimpleHashMap<const BSPMaterial *, PT( ShaderConfig ), pointer_hash> ConfigCache;
        ConfigCache _config_cache;
        typedef pmap<ShaderPermutations, CPT( ShaderAttrib )> GeneratedShaders;
        GeneratedShaders _generated_shaders;
        
        Filename _vertex_file;
        Filename _pixel_file;
        Filename _geom_file;
        std::string _vertex_source;
        std::string _pixel_source;
        std::string _geom_source;
        bool _has_vertex;
        bool _has_pixel;
        bool _has_geom;

        static TypeHandle get_class_type()
        {
                return _type_handle;
        }
        static void init_type()
        {
                ReferenceCount::init_type();
                Namable::init_type();
                register_type( _type_handle, "ShaderSpec",
                               ReferenceCount::get_class_type(),
                               Namable::get_class_type() );
        }

private:
        static TypeHandle _type_handle;
};

#endif // SHADER_SPEC_H
