#include "shader_decalmodulate.h"
#include <colorBlendAttrib.h>

DecalModulateSpec::DecalModulateSpec() :
	UnlitGenericSpec()
{
	set_name( "DecalModulate" );
}

void DecalModulateSpec::setup_permutations( ShaderPermutations &result,
	const BSPMaterial *mat,
	const RenderState *state,
	const GeomVertexAnimationSpec &anim,
	BSPShaderGenerator *generator )
{
	UnlitGenericSpec::setup_permutations( result, mat, state, anim, generator );
	ULGConfig *conf = (ULGConfig *)get_shader_config( mat );
	enable_srgb_read( conf->basetexture.base_texture, false );
	result.add_permutation( "IS_DECAL_MODULATE" );
}
