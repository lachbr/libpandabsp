/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file shader_unlitnomat.h
 * @author Brian Lach
 * @date January 11, 2019
 */

#ifndef SHADER_UNLITNOMAT_H
#define SHADER_UNLITNOMAT_H

#include "shader_spec.h"

class EXPCL_PANDABSP UNMConfig : public ShaderConfig
{
public:
        virtual void parse_from_material_keyvalues( const BSPMaterial *mat );
};

class EXPCL_PANDABSP UnlitNoMatSpec : public ShaderSpec
{
PUBLISHED:
        UnlitNoMatSpec();

public:
	virtual void setup_permutations( ShaderPermutations &perms, const BSPMaterial *mat, const RenderState *state,
		const GeomVertexAnimationSpec &anim, BSPShaderGenerator *generator );
        virtual PT( ShaderConfig ) make_new_config();
};

#endif // SHADER_UNLITNOMAT_H
