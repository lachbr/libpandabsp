#ifndef SHADER_CSMRENDER_H
#define SHADER_CSMRENDER_H

#include "shader_spec.h"
#include "shader_features.h"

class EXPCL_PANDABSP CSMRenderConfig : public ShaderConfig
{
public:
        virtual void parse_from_material_keyvalues( const BSPMaterial *mat );

        BaseTextureFeature basetexture;
        AlphaFeature alpha;
};

class EXPCL_PANDABSP CSMRenderSpec : public ShaderSpec
{
PUBLISHED:
        CSMRenderSpec();

public:
	virtual void setup_permutations( ShaderPermutations &perms, const BSPMaterial *mat, const RenderState *state,
		const GeomVertexAnimationSpec &anim, BSPShaderGenerator *generator );
        virtual PT( ShaderConfig ) make_new_config();
};

#endif // SHADER_CSMRENDER_H