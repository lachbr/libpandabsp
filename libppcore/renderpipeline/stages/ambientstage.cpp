#include "ambientStage.h"

#include "..\..\pp_globals.h"

vector_string AmbientStage::
get_required_inputs()
{
        vector_string vec;
        vec.push_back( "DefaultEnvmap" );
        vec.push_back( "PrefilteredBRDF" );
        vec.push_back( "PrefilteredMetalBRDF" );
        vec.push_back( "PrefilteredCoatBRDF" );
        return vec;
}

vector_string AmbientStage::
get_required_pipes()
{
        vector_string vec;
        vec.push_back( "ShadedScene" );
        vec.push_back( "GBuffer" );
        return vec;
}

AmbientStage::
AmbientStage() :
        RenderStage()
{
}

void AmbientStage::
create()
{
        _target = create_target( "AmbientStage" );
        _target->add_color_attachment( 16 );
        _target->prepare_buffer();
}

void AmbientStage::
bind()
{
        // required pipes
        rpipeline->_gbuf_stage->_ubo.bind_to( _target ); // GBuffer
        // There are multiple produced pipes shadedscenes so I'm just going to use the one here.
        // I don't really know lol.
        set_shader_input( new ShaderInput( "ShadedScene", _target->get_color_tex() ) );

        // required inputs
        set_shader_input( new ShaderInput( "DefaultEnvmap", rpipeline->_def_envmap ) );
        set_shader_input( new ShaderInput( "PrefilteredBRDF", rpipeline->_pref_brdf ) );
        set_shader_input( new ShaderInput( "PrefilteredMetalBRDF", rpipeline->_pref_metal_brdf ) );
        set_shader_input( new ShaderInput( "PrefilteredCoatBRDF", rpipeline->_pref_coat_brdf ) );

        // common inputs
        bind_to_commons();
}

void AmbientStage::
reload_shaders()
{
        _target->set_shader( load_shader( "shader/ambient_stage.frag.glsl" ) );
}