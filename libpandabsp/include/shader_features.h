/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file shader_features.h
 * @author Brian Lach
 * @date December 26, 2018
 */

#ifndef SHADER_FEATURES_H
#define SHADER_FEATURES_H

#include <texture.h>
#include <aa_luse.h>

class BSPMaterial;
class ShaderConfig;
class ShaderPermutations;

enum shaderfeatureflags_t
{
        SF_BASETEXTURE,
        SF_ALPHA,
        SF_TRANSLUCENT,
        SF_HALFLAMBERT,
        SF_LIGHTWARP,
        SF_PHONG,
        SF_ENVMAP,
        SF_DETAIL,
        SF_BUMPMAP,
};

class ShaderFeature
{
public:
        INLINE ShaderFeature() :
                has_feature( false )
        {
        }

        virtual void parse_from_material_keyvalues( const BSPMaterial *mat, ShaderConfig *conf ) = 0;
        virtual void add_permutations( ShaderPermutations &perms ) = 0;

        bool has_feature;
};

class BaseTextureFeature : public ShaderFeature
{
public:
        INLINE BaseTextureFeature() :
                ShaderFeature(),
                base_texture( nullptr )
        {
        }

        virtual void parse_from_material_keyvalues( const BSPMaterial *mat, ShaderConfig *conf );
        virtual void add_permutations( ShaderPermutations &perms );

        PT( Texture ) base_texture;
};

class AlphaFeature : public ShaderFeature
{
public:
        INLINE AlphaFeature() :
                ShaderFeature(),
                alpha( -1 ),
                translucent( false )
        {
        }

        virtual void parse_from_material_keyvalues( const BSPMaterial *mat, ShaderConfig *conf );
        virtual void add_permutations( ShaderPermutations &perms );

        float alpha;
        bool translucent;
};

class HalfLambertFeature : public ShaderFeature
{
public:
        INLINE HalfLambertFeature()
                : ShaderFeature(),
                halflambert( true )
        {
                has_feature = true;
        }

        virtual void parse_from_material_keyvalues( const BSPMaterial *mat, ShaderConfig *conf );
        virtual void add_permutations( ShaderPermutations &perms );

        bool halflambert;
};

class PhongFeature : public ShaderFeature
{
public:
        INLINE PhongFeature() :
                ShaderFeature(),
                phong_exponent( 1.0 ),
                phong_exponent_texture( nullptr ),
                phong_boost( 1.0 ),
                phong_fresnel_ranges( 0.0, 0.5, 1.0 ),
                phong_albedo_tint( false ),
                phong_tint( 1.0, 1.0, 1.0 )
        {
        }

        virtual void parse_from_material_keyvalues( const BSPMaterial *mat, ShaderConfig *conf );
        virtual void add_permutations( ShaderPermutations &perms );

        // phong
        float phong_exponent;
        PT( Texture ) phong_exponent_texture;
        float phong_boost;
        LVector3 phong_fresnel_ranges;
        bool phong_albedo_tint;
        LVector3 phong_tint;
};

class EnvmapFeature : public ShaderFeature
{
public:
        INLINE EnvmapFeature() :
                ShaderFeature(),
                envmap_texture( nullptr ),
                envmap_mask_texture( nullptr ),
                envmap_tint( 1.0 ),
                envmap_contrast( 0.0 ),
                envmap_saturation( 1.0 )
        {
        }

        virtual void parse_from_material_keyvalues( const BSPMaterial *mat, ShaderConfig *conf );
        virtual void add_permutations( ShaderPermutations &perms );

        // envmap
        PT( Texture ) envmap_texture;
        PT( Texture ) envmap_mask_texture;
        LVector3 envmap_tint;
        LVector3 envmap_contrast;
        LVector3 envmap_saturation;
};

class DetailFeature : public ShaderFeature
{
public:
        INLINE DetailFeature() :
                ShaderFeature(),
                detail_texture( nullptr ),
                detail_factor( 1.0 ),
                detail_scale( 1.0 ),
                detail_tint( 1.0 )
        {
        }

        virtual void parse_from_material_keyvalues( const BSPMaterial *mat, ShaderConfig *conf );
        virtual void add_permutations( ShaderPermutations &perms );

        // detail
        PT( Texture ) detail_texture;
        float detail_factor;
        float detail_scale;
        LVector3 detail_tint;
};

class BumpmapFeature : public ShaderFeature
{
public:
        INLINE BumpmapFeature() :
                ShaderFeature(),
                bump_tex( nullptr )
        {
        }

        virtual void parse_from_material_keyvalues( const BSPMaterial *mat, ShaderConfig *conf );
        virtual void add_permutations( ShaderPermutations &perms );

        PT( Texture ) bump_tex;
};

class LightwarpFeature : public ShaderFeature
{
public:
        INLINE LightwarpFeature() :
                ShaderFeature(),
                lightwarp_tex( nullptr )
        {
        }

        virtual void parse_from_material_keyvalues( const BSPMaterial *mat, ShaderConfig *conf );
        virtual void add_permutations( ShaderPermutations &perms );

        PT( Texture ) lightwarp_tex;
};

#endif // SHADER_FEATURES_H