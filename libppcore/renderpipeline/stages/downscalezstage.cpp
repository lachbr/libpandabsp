#include "downscaleZStage.h"

#include "..\..\pp_globals.h"

vector_string DownscaleZStage::get_required_inputs()
{
        vector_string vec;
        return vec;
}

vector_string DownscaleZStage::get_required_pipes()
{
        vector_string vec;
        vec.push_back( "GBuffer" );
        return vec;
}

DownscaleZStage::DownscaleZStage() :
        RenderStage()
{
}

void DownscaleZStage::create()
{
        _target = create_target( "DownscaleDepth" );
        _target->add_color_attachment( LVecBase4i( 16, 0, 0, 0 ) );
        _target->prepare_buffer();
}

void DownscaleZStage::bind()
{
        rpipeline->_gbuf_stage->_ubo.bind_to( _target ); // GBuffer
        bind_to_commons();
}

void DownscaleZStage::reload_shaders()
{
        _target->set_shader( load_shader( "shader/downscale_depth.frag.glsl" ) );
}