#include "shader_csmrender.h"
#include "shader_generator.h"
#include "pssmCameraRig.h"

void CSMRenderConfig::parse_from_material_keyvalues( const BSPMaterial *mat )
{
        basetexture.parse_from_material_keyvalues( mat, this );
        alpha.parse_from_material_keyvalues( mat, this );
}

CSMRenderSpec::CSMRenderSpec() :
        ShaderSpec( "CSMRender",
                    Filename( "shaders/stdshaders/pssm_camera.vert.glsl" ),
                    Filename( "shaders/stdshaders/pssm_camera.frag.glsl" ),
                    Filename( "shaders/stdshaders/pssm_camera.geom.glsl" ) )
{
}

void CSMRenderSpec::setup_permutations( ShaderPermutations &result,
	const BSPMaterial *mat,
	const RenderState *state,
	const GeomVertexAnimationSpec &anim,
	BSPShaderGenerator *generator )
{
	ShaderSpec::setup_permutations( result, mat, state, anim, generator );

        CSMRenderConfig *conf = (CSMRenderConfig *)get_shader_config( mat );

        conf->basetexture.add_permutations( result );
        conf->alpha.add_permutations( result );

        add_hw_skinning( anim, result );

        result.add_input( ShaderInput( "split_mvps", generator->get_pssm_rig()->get_mvp_array() ) );
}

PT( ShaderConfig ) CSMRenderSpec::make_new_config()
{
        return new CSMRenderConfig;
}