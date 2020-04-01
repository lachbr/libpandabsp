#ifndef SHADER_SKYBOX_H
#define SHADER_SKYBOX_H

#include "shader_spec.h"

class EXPCL_PANDABSP SkyBoxConfig : public ShaderConfig
{
public:
        virtual void parse_from_material_keyvalues( const BSPMaterial *mat );
};

class EXPCL_PANDABSP SkyBoxSpec : public ShaderSpec
{
PUBLISHED:
        SkyBoxSpec();

public:
	virtual void setup_permutations( ShaderPermutations &perms, const BSPMaterial *mat, const RenderState *state,
		const GeomVertexAnimationSpec &anim, BSPShaderGenerator *generator );
        virtual PT( ShaderConfig ) make_new_config();
};

#endif // SHADER_SKYBOX_H