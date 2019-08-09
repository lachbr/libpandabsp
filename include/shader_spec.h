/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
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

#include <unordered_map>

#define SHADER_PERMS_UNORDERED_MAP

class RenderState;
class BSPShaderGenerator;
class BSPMaterial;

struct ShaderPrecacheCombo_t
{
	std::string combo_name;
	int min_val;
	int max_val;
	bool is_bool;
};

struct ShaderPrecacheComboCondition_t
{
	std::string combo_name;
	int val;
};

struct ShaderPrecacheComboSkipCondition_t
{
	pvector<ShaderPrecacheComboCondition_t> conditions;
};

class ShaderPrecacheCombos
{
public:
	void add( const std::string &name, int min, int max )
	{
		combos.push_back( { name, min, max, false } );
	}
	void add_bool( const std::string &name, bool always_true = false )
	{
		if ( always_true )
			combos.push_back( { name, 1, 1, true } );
		else
			combos.push_back( { name, 0, 1, true } );
	}
	void skip( const ShaderPrecacheComboSkipCondition_t &skip )
	{
		skips.push_back( skip );
	}
	pvector<ShaderPrecacheCombo_t> combos;
	pvector<ShaderPrecacheComboSkipCondition_t> skips;
};

class ShaderConfig : public ReferenceCount
{
public:
        virtual void parse_from_material_keyvalues( const BSPMaterial *mat ) = 0;
};

/**
 * Represents a list of #defines and variable inputs to a shader that is being generated.
 */
class ShaderPermutations : public ReferenceCount
{
#ifdef SHADER_PERMS_UNORDERED_MAP
public:
	class Hasher
	{
	public:
		INLINE size_t operator ()( const CPT( ShaderPermutations ) &perms ) const
		{
			return perms->hash;
		}
	};

	class Compare
	{
	public:
		INLINE bool operator ()( const CPT( ShaderPermutations ) &a, const CPT( ShaderPermutations ) &b ) const
		{
			return a->hash == b->hash;
		}
	};
#endif

public:
	std::ostringstream permutations_stream;
	std::string permutations;

	int flags;
	vector_int flag_indices;

	pvector<ShaderInput> inputs;

	size_t hash;

PUBLISHED:

	INLINE ShaderPermutations() :
		ReferenceCount()
	{
		// This should be enough for most shaders
		inputs.reserve( 32 );
		flag_indices.reserve( 32 );
		hash = 0u;
		flags = 0;
	}

        template<class T>
        INLINE void add_permutation( const std::string &key, const T &value )
        {
		permutations_stream << "#define " << key << " " << value << "\n";
        }

	INLINE void add_permutation( const std::string &key, const std::string &value = "1" )
	{
		add_permutation<std::string>( key, value );
	}

	INLINE void complete()
	{
		permutations = permutations_stream.str();
		hash = string_hash::add_hash( hash, permutations );
		hash = int_hash::add_hash( hash, flags );
	}

	INLINE void add_input( const ShaderInput &inp )
        {
		hash = inp.add_hash( hash );
		inputs.push_back( inp );
        }

        INLINE void add_flag( int flag )
        {
		flags |= ( 1 << flag );
		flag_indices.push_back( flag );
        }

        INLINE size_t get_hash() const
        {
		return hash;
        }
};

/**
 * Serves to setup the permutations for a specific shader
 * when setting up a shader for a specific RenderState.
 */
class ShaderSpec : public ReferenceCount, public Namable
{
public:
        struct ShaderSource
        {
                Filename filename;
                std::string full_source;
                std::string before_defines;
                std::string after_defines;
                bool has;

                INLINE ShaderSource() :
                        has( false )
                {
                }

                void read( const Filename &f );
        };
PUBLISHED:
        ShaderSpec( const std::string &name, const Filename &vert_file,
                    const Filename &pixel_file, const Filename &geom_file = "" );

        void read_shader_files( const Filename &vert_file,
                                const Filename &pixel_file, const Filename &geom_file );

public:
	virtual void setup_permutations( ShaderPermutations &perms, const BSPMaterial *mat, const RenderState *state,
		const GeomVertexAnimationSpec &anim, BSPShaderGenerator *generator );

	virtual void add_precache_combos( ShaderPrecacheCombos &combos );
	virtual void precache();

        ShaderConfig *get_shader_config( const BSPMaterial *mat );
        virtual PT( ShaderConfig ) make_new_config() = 0;

        static bool add_fog( const RenderState *rs, ShaderPermutations &perms, BSPShaderGenerator *generator );
        static void add_color( const RenderState *rs, ShaderPermutations &perms );
        static bool add_csm( const RenderState *rs, ShaderPermutations &perms, BSPShaderGenerator *generator );
        static bool add_clip_planes( const RenderState *rs, ShaderPermutations &perms );
        static void add_hw_skinning( const GeomVertexAnimationSpec &anim, ShaderPermutations &perms );
	static bool add_alpha_test( const RenderState *rs, ShaderPermutations &perms );
	static bool add_aux_bits( const RenderState *rs, ShaderPermutations &perms );

        typedef SimpleHashMap<const BSPMaterial *, PT( ShaderConfig ), pointer_hash> ConfigCache;
        ConfigCache _config_cache;

#ifdef SHADER_PERMS_UNORDERED_MAP
	typedef std::unordered_map<CPT( ShaderPermutations ), CPT( ShaderAttrib ), ShaderPermutations::Hasher, ShaderPermutations::Compare> GeneratedShaders;
#else
	typedef indirect_method_hash<const ShaderPermutations *, std::equal_to<const ShaderPermutations *>> ShaderPermutationHashMethod;
	typedef SimpleHashMap<CPT( ShaderPermutations ), CPT( ShaderAttrib ), ShaderPermutationHashMethod> GeneratedShaders;
#endif
        GeneratedShaders _generated_shaders;
        
        ShaderSource _vertex;
        ShaderSource _pixel;
        ShaderSource _geom;

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
	void r_precache( ShaderPrecacheCombos &combos );

        static TypeHandle _type_handle;
};

void enable_srgb_read( Texture *tex, bool enable );

#endif // SHADER_SPEC_H
