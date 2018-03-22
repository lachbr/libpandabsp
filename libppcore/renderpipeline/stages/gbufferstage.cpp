#include "gBufferStage.h"

#include "..\..\pp_globals.h"

vector_string GBufferStage::get_required_inputs()
{
        vector_string vec;
        vec.push_back( "DefaultEnvmap" );
        return vec;
}

vector_string GBufferStage::get_required_pipes()
{
        vector_string vec;
        return vec;
}

GBufferStage::GBufferStage() :
        RenderStage()
{
}

void GBufferStage::make_gbuffer_ubo()
{
        _ubo.add_input( new ShaderInput( "GBuffer.Depth", _target->get_depth_tex() ) );
        _ubo.add_input( new ShaderInput( "GBuffer.Data0", _target->get_color_tex() ) );
        _ubo.add_input( new ShaderInput( "GBuffer.Data1", _target->get_aux_tex( 0 ) ) );
        _ubo.add_input( new ShaderInput( "GBuffer.Data2", _target->get_aux_tex( 1 ) ) );
}

void GBufferStage::create()
{
        make_gbuffer_ubo();

        _target = create_target( "GBuffer" );
        _target->add_color_attachment( 16, true );
        _target->add_depth_attachment( 32 );
        _target->add_aux_attachments( 16, 2 );
        _target->prepare_render( base->_window->get_camera_group() );
}

void GBufferStage::bind()
{
        set_shader_input( new ShaderInput( "DefaultEnvmap", rpipeline->_def_envmap ) );

        bind_to_commons();
}

void GBufferStage::set_shader_input( const ShaderInput *inp )
{
        render.set_shader_input( inp );
}