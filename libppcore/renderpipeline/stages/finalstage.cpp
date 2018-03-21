#include "finalStage.h"

#include "..\..\pp_globals.h"

vector_string FinalStage::
get_required_inputs()
{
        vector_string vec;
        return vec;
}

vector_string FinalStage::
get_required_pipes()
{
        vector_string vec;
        vec.push_back( "ShadedScene" );
        return vec;
}

FinalStage::
FinalStage() :
        RenderStage()
{
}

void FinalStage::
create()
{
        _target = create_target( "FinalStage" );
        _target->add_color_attachment( 16 );
        _target->prepare_buffer();

        // XXX: We cannot simply assign the final shader to the window display
        // region. This is because of a bug that the last FBO always only gets
        // 8 bits, regardles of what was requested - probably because of the
        // assumption that no tonemapping / srgb correction will follow afterwards.
        //
        // This might be a driver bug, or an optimization in Panda3D.However, it
        // also has the nice side effect that when taking screenshots(or using
        // the pixel inspector), we get the srgb corrected data, so its not too
        // much of a disadvantage.
        _present_target = create_target( "FinalPresentStage" );
        _present_target->present_on_screen();
        _present_target->set_shader_input( new ShaderInput( "SourceTex", _target->get_color_tex() ) );
}

void FinalStage::
bind()
{
        set_shader_input( new ShaderInput( "ShadedScene", _target->get_color_tex() ) );

        bind_to_commons();
}

void FinalStage::
reload_shaders()
{
        _target->set_shader( load_shader( "shader/final_stage.frag.glsl" ) );
        _present_target->set_shader( load_shader( "shader/final_present_stage.frag.glsl" ) );
}