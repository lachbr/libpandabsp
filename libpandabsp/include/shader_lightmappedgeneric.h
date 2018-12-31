/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file shader_lightmappedgeneric.h
 * @author Brian Lach
 * @date December 26, 2018
 */

#ifndef SHADER_LIGHTMAPPEDGENERIC_H
#define SHADER_LIGHTMAPPEDGENERIC_H

#include "shader_spec.h"
#include "shader_features.h"

class LMGConfig : public ShaderConfig
{
public:
        virtual void parse_from_material_keyvalues( const BSPMaterial *mat );

        BaseTextureFeature basetexture;
        AlphaFeature alpha;
        EnvmapFeature envmap;
        BumpmapFeature bumpmap;
        DetailFeature detail;
};

class LightmappedGenericSpec : public ShaderSpec
{
PUBLISHED:
        LightmappedGenericSpec();

public:
        virtual ShaderPermutations setup_permutations( const BSPMaterial *mat,
                                                 const RenderState *rs,
                                                 const GeomVertexAnimationSpec &anim,
                                                 PSSMShaderGenerator *generator );
        virtual PT( ShaderConfig ) make_new_config();

};

#endif // SHADER_LIGHTMAPPEDGENERIC_H