#include "shader_decalmodulate.h"
#include <colorBlendAttrib.h>

DecalModulateSpec::DecalModulateSpec() :
	UnlitGenericSpec()
{
	set_name( "DecalModulate" );
}

ShaderPermutations DecalModulateSpec::setup_permutations( const BSPMaterial *mat,
	const RenderState *rs,
	const GeomVertexAnimationSpec &anim,
	BSPShaderGenerator *generator )
{
	ShaderPermutations result = UnlitGenericSpec::setup_permutations( mat, rs, anim, generator );

	result.add_permutation( "IS_DECAL_MODULATE" );

	return result;
}