#include "shader_skybox.h"
#include "shader_generator.h"

void SkyBoxConfig::parse_from_material_keyvalues( const BSPMaterial *mat )
{
}

SkyBoxSpec::SkyBoxSpec() :
        ShaderSpec( "SkyBox", Filename( "phase_14/models/shaders/stdshaders/skybox.vert.glsl" ),
                Filename( "phase_14/models/shaders/stdshaders/skybox.frag.glsl" ) )
{
}

void SkyBoxSpec::setup_permutations( ShaderPermutations &result,
	const BSPMaterial *mat,
	const RenderState *rs,
	const GeomVertexAnimationSpec &anim,
	BSPShaderGenerator *generator )
{
	ShaderSpec::setup_permutations( result, mat, rs, anim, generator );

        result.add_input( ShaderInput( "skyboxSampler", generator->get_identity_cubemap() ) );

	add_aux_bits( rs, result );
}

PT( ShaderConfig ) SkyBoxSpec::make_new_config()
{
        return new SkyBoxConfig;
}