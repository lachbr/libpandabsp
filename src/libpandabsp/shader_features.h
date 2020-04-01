/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file shader_features.h
 * @author Brian Lach
 * @date December 26, 2018
 */

#ifndef SHADER_FEATURES_H
#define SHADER_FEATURES_H

#include <texture.h>
#include <aa_luse.h>

#include "config_bsp.h"

class BSPMaterial;
class ShaderConfig;
class ShaderPermutations;

// New material parameters for PBR
//
// Textures
//      $basetexture    -       Diffuse map RGBA, if selfillum, A is selfillum mask
//      $bumpmap        -       Normal map
//      $specgloss      -       Specular RGB, Glossiness A
// Parameters
//      $envmap         -       Cubemap specular reflection, `env_cubemap` uses closest one in level,
//                              or provide an explicit cubemap texture
//      $phong          -       Real-time diffuse reflection from local lights, `1` to enable
//      $selfillum      -       Self-illumination/glow, `1` to enable, will use Alpha channel
//                              of $basetexture as a mask

#define DECLARE_SHADERFEATURE() \
public:\
        virtual void parse_from_material_keyvalues( const BSPMaterial *mat, ShaderConfig *conf );\
        virtual void add_permutations( ShaderPermutations &perms );

#define SHADERFEATURE_PARSE_FUNC(clsname)\
void clsname::parse_from_material_keyvalues(const BSPMaterial *mat, ShaderConfig *conf)

#define SHADERFEATURE_SETUP_FUNC(clsname)\
void clsname::add_permutations(ShaderPermutations &perms)

class EXPCL_PANDABSP ShaderFeature
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

/**
 * Diffuse/albedo map. If $selfillum is 1, alpha channel
 * is the selfillum mask.
 */
class EXPCL_PANDABSP BaseTextureFeature : public ShaderFeature
{
        DECLARE_SHADERFEATURE();
public:
        INLINE BaseTextureFeature() :
                ShaderFeature(),
                base_texture( nullptr )
        {
        }

        PT( Texture ) base_texture;
};

/**
 * A normal map.
 */
class EXPCL_PANDABSP BumpmapFeature : public ShaderFeature
{
        DECLARE_SHADERFEATURE();
public:
        INLINE BumpmapFeature() :
                ShaderFeature(),
                bump_tex( nullptr )
        {
        }

        PT( Texture ) bump_tex;
};

/**
 * AO/Roughness/Metallic/Emissive texture
 * Each channel represents each piece of data.
 *
 * Or explicit values for each.
 */
class EXPCL_PANDABSP ARME_Feature : public ShaderFeature
{
        DECLARE_SHADERFEATURE();
public:
        INLINE ARME_Feature() :
                ShaderFeature(),
                arme_texture( nullptr ),
                ao( 1.0 ),
                roughness( 0.0 ),
                metallic( 0.0 ),
                emissive( 0.0 )
        {
                has_feature = true;
        }

        PT( Texture ) arme_texture;
        float ao;
        float roughness;
        float metallic;
        float emissive;
};

/**
 * Specifies if the material has self-illuminated (emissive)
 * parts. If so, the emissive mask is read from the Alpha
 * channel of the basetexture. Since the mask is only one
 * channel, an optional $selfillumtint can be specified.
 */
class EXPCL_PANDABSP SelfIllumFeature : public ShaderFeature
{
        DECLARE_SHADERFEATURE();
public:
        INLINE SelfIllumFeature() :
                ShaderFeature(),
                selfillumtint( 1.0, 1.0, 1.0 )
        {
        }

        LVector3 selfillumtint;
};

/**
 * Specifies if the material has translucent parts.
 * If translucent, alpha value is read from $basetexture.
 * Or provides an explicit alpha value.
 */
class EXPCL_PANDABSP AlphaFeature : public ShaderFeature
{
        DECLARE_SHADERFEATURE();

public:
        INLINE AlphaFeature() :
                ShaderFeature(),
                alpha( -1 ),
                translucent( false )
        {
        }        

        float alpha;
        bool translucent;
};

/**
 * Texture which warps the NdotL term.
 */
class EXPCL_PANDABSP LightwarpFeature : public ShaderFeature
{
        DECLARE_SHADERFEATURE();
public:
        INLINE LightwarpFeature() :
                ShaderFeature(),
                lightwarp_tex( nullptr )
        {
        }

        PT( Texture ) lightwarp_tex;
};

/**
 * Specifies if the material should use the half-lambert warping function
 * of the NdotL term.
 */
class EXPCL_PANDABSP HalfLambertFeature : public ShaderFeature
{
        DECLARE_SHADERFEATURE();
public:
        INLINE HalfLambertFeature()
                : ShaderFeature(),
                halflambert( false )
        {
        }

        bool halflambert;
};

/**
 * A dedicated rim term derived from Team Fortess 2 shading.
 * Gives extra highlights to the outer edges near the top of
 * an object to help them stand out more against the environment.
 */
class EXPCL_PANDABSP RimLightFeature : public ShaderFeature
{
        DECLARE_SHADERFEATURE();
public:
        INLINE RimLightFeature() :
                ShaderFeature(),
                boost( 1.0 ),
                exponent( 4.0 )
        {
        }

        float boost;
        float exponent;
};

class EXPCL_PANDABSP EnvmapFeature : public ShaderFeature
{
        DECLARE_SHADERFEATURE();
public:
        INLINE EnvmapFeature() :
                ShaderFeature(),
                envmap_tint( 1.0 ),
                envmap_texture( nullptr )
        {
        }

        PT( Texture ) envmap_texture;
        LVector3 envmap_tint;
};

/**
 * Not used anywhere yet, but gives higher detail up close
 * to a surface. Derived from Half-Life 2.
 */
class EXPCL_PANDABSP DetailFeature : public ShaderFeature
{
        DECLARE_SHADERFEATURE();
public:
        INLINE DetailFeature() :
                ShaderFeature(),
                detail_texture( nullptr ),
                detail_factor( 1.0 ),
                detail_scale( 1.0 ),
                detail_tint( 1.0 )
        {
        }

        // detail
        PT( Texture ) detail_texture;
        float detail_factor;
        float detail_scale;
        LVector3 detail_tint;
};

#endif // SHADER_FEATURES_H
