/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file shader_unlitnomat.h
 * @author Brian Lach
 * @date January 11, 2019
 */

#ifndef SHADER_UNLITNOMAT_H
#define SHADER_UNLITNOMAT_H

#include "shader_spec.h"

class UNMConfig : public ShaderConfig
{
public:
        virtual void parse_from_material_keyvalues( const BSPMaterial *mat );
};

class UnlitNoMatSpec : public ShaderSpec
{
PUBLISHED:
        UnlitNoMatSpec();

public:
        virtual ShaderPermutations setup_permutations( const BSPMaterial *mat,
                                                       const RenderState *rs,
                                                       const GeomVertexAnimationSpec &anim,
                                                       BSPShaderGenerator *generator );
        virtual PT( ShaderConfig ) make_new_config();
};

#endif // SHADER_UNLITNOMAT_H