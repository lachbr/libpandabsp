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
                    Filename( "phase_14/models/shaders/pssm_camera.vert.glsl" ),
                    Filename( "phase_14/models/shaders/pssm_camera.frag.glsl" ),
                    Filename( "phase_14/models/shaders/pssm_camera.geom.glsl" ) )
{
}

ShaderPermutations CSMRenderSpec::setup_permutations( const BSPMaterial *mat,
                                                      const RenderState *rs,
                                                      const GeomVertexAnimationSpec &anim,
                                                      PSSMShaderGenerator *generator )
{
        CSMRenderConfig *conf = (CSMRenderConfig *)get_shader_config( mat );

        ShaderPermutations result = ShaderSpec::setup_permutations( mat, rs, anim, generator );

        conf->basetexture.add_permutations( result );
        conf->alpha.add_permutations( result );

        add_hw_skinning( anim, result );

        result.add_input( ShaderInput( "split_mvps", generator->get_pssm_rig()->get_mvp_array() ) );

        return result;
}

PT( ShaderConfig ) CSMRenderSpec::make_new_config()
{
        return new CSMRenderConfig;
}