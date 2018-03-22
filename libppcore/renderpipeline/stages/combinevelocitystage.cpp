#include "combineVelocityStage.h"

#include "..\..\pp_globals.h"

vector_string CombineVelocityStage::get_required_inputs()
{
        vector_string vec;
        return vec;
}

vector_string CombineVelocityStage::get_required_pipes()
{
        vector_string vec;
        vec.push_back( "GBuffer" );
        return vec;
}

CombineVelocityStage::CombineVelocityStage() :
        RenderStage()
{
}

void CombineVelocityStage::create()
{
        _target = create_target( "CombineVelocity" );
        _target->add_color_attachment( LVecBase4i( 16, 16, 0, 0 ) );
        _target->prepare_buffer();
}

void CombineVelocityStage::bind()
{
        rpipeline->_gbuf_stage->_ubo.bind_to( _target );

        bind_to_commons();
}

void CombineVelocityStage::reload_shaders()
{
        _target->set_shader( load_shader( "combine_velocity.frag.glsl" ) );
}