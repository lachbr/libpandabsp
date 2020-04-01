#ifndef SHADER_DECALMODULATE_H
#define SHADER_DECALMODULATE_H

#include "shader_unlitgeneric.h"

/**
 * This is just an alias of UnlitGeneric that enables color blending on the decal.
 */
class EXPCL_PANDABSP DecalModulateSpec : public UnlitGenericSpec
{
PUBLISHED:
	DecalModulateSpec();

public:
	virtual void setup_permutations( ShaderPermutations &perms, const BSPMaterial *mat, const RenderState *state,
		const GeomVertexAnimationSpec &anim, BSPShaderGenerator *generator );
};

#endif // SHADER_DECALMODULATE_H