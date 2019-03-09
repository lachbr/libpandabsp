#ifndef SHADER_SKYBOX_H
#define SHADER_SKYBOX_H

#include "shader_spec.h"

class SkyBoxConfig : public ShaderConfig
{
public:
        virtual void parse_from_material_keyvalues( const BSPMaterial *mat );
};

class SkyBoxSpec : public ShaderSpec
{
PUBLISHED:
        SkyBoxSpec();

public:
        virtual ShaderPermutations setup_permutations( const BSPMaterial *mat,
                const RenderState *rs,
                const GeomVertexAnimationSpec &anim,
                PSSMShaderGenerator *generator );
        virtual PT( ShaderConfig ) make_new_config();
};

#endif // SHADER_SKYBOX_H