/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file shader_vertexlitgeneric.h
 * @author Brian Lach
 * @date December 26, 2018
 */

#ifndef SHADER_VERTEXLITGENERIC_H
#define SHADER_VERTEXLITGENERIC_H

#include "shader_spec.h"
#include "shader_features.h"

class VLGShaderConfig : public ShaderConfig
{
public:

        BaseTextureFeature basetexture;
        BumpmapFeature bumpmap;
        EnvmapFeature envmap;
        PhongFeature phong;
        DetailFeature detail;
        AlphaFeature alpha;
        HalfLambertFeature halflambert;
        LightwarpFeature lightwarp;
        RimLightFeature rimlight;
        SelfIllumFeature selfillum;

        virtual void parse_from_material_keyvalues( const BSPMaterial *mat );
};

class VertexLitGenericSpec : public ShaderSpec
{
PUBLISHED:
        VertexLitGenericSpec();

public:
        virtual ShaderPermutations setup_permutations( const BSPMaterial *mat,
                                                 const RenderState *rs,
                                                 const GeomVertexAnimationSpec &anim,
                                                 PSSMShaderGenerator *generator );

        virtual PT( ShaderConfig ) make_new_config();
};

#endif // SHADER_VERTEXLITGENERIC_H