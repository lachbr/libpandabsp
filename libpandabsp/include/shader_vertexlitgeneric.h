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
        ARME_Feature arme;
        BumpmapFeature bumpmap;
        EnvmapFeature envmap;
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
	virtual void setup_permutations( ShaderPermutations &perms, const BSPMaterial *mat, const RenderState *state,
		const GeomVertexAnimationSpec &anim, BSPShaderGenerator *generator );

        virtual PT( ShaderConfig ) make_new_config();

	virtual void add_precache_combos( ShaderPrecacheCombos &combos );
};

#endif // SHADER_VERTEXLITGENERIC_H