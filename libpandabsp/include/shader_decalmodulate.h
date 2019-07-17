#ifndef SHADER_DECALMODULATE_H
#define SHADER_DECALMODULATE_H

#include "shader_unlitgeneric.h"

/**
 * This is just an alias of UnlitGeneric that enables color blending on the decal.
 */
class DecalModulateSpec : public UnlitGenericSpec
{
PUBLISHED:
	DecalModulateSpec();

public:
	virtual ShaderPermutations setup_permutations( const BSPMaterial *mat,
		const RenderState *rs,
		const GeomVertexAnimationSpec &anim,
		BSPShaderGenerator *generator );
};

#endif // SHADER_DECALMODULATE_H