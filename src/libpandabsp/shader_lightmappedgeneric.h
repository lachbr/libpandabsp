/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file shader_lightmappedgeneric.h
 * @author Brian Lach
 * @date December 26, 2018
 */

#ifndef SHADER_LIGHTMAPPEDGENERIC_H
#define SHADER_LIGHTMAPPEDGENERIC_H

#include "shader_spec.h"
#include "shader_features.h"

class EXPCL_PANDABSP LMGConfig : public ShaderConfig
{
public:
        virtual void parse_from_material_keyvalues( const BSPMaterial *mat );

        BaseTextureFeature basetexture;
        AlphaFeature alpha;
        ARME_Feature arme;
        EnvmapFeature envmap;
        BumpmapFeature bumpmap;
        DetailFeature detail;

	bool _uses_planar_reflection;
};

class EXPCL_PANDABSP LightmappedGenericSpec : public ShaderSpec
{
PUBLISHED:
        LightmappedGenericSpec();

public:
	virtual void setup_permutations( ShaderPermutations &perms, const BSPMaterial *mat, const RenderState *state,
		const GeomVertexAnimationSpec &anim, BSPShaderGenerator *generator );
        virtual PT( ShaderConfig ) make_new_config();

};

#endif // SHADER_LIGHTMAPPEDGENERIC_H
