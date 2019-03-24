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

ShaderPermutations SkyBoxSpec::setup_permutations( const BSPMaterial *mat,
        const RenderState *rs,
        const GeomVertexAnimationSpec &anim,
        BSPShaderGenerator *generator )
{
        ShaderPermutations result = ShaderSpec::setup_permutations( mat, rs, anim, generator );

        result.add_input( ShaderInput( "skyboxRTT", generator->get_skybox_rtt() ) );

        return result;
}

PT( ShaderConfig ) SkyBoxSpec::make_new_config()
{
        return new SkyBoxConfig;
}