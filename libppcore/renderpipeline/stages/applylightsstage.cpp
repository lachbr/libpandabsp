#include "applyLightsStage.h"

#include "..\..\pp_globals.h"

vector_string ApplyLightsStage::
get_required_inputs()
{
        vector_string vec;
        vec.push_back( "AllLightsData" );
        vec.push_back( "IESDatasetTex" );
        vec.push_back( "ShadowSourceData" );
        return vec;
}

vector_string ApplyLightsStage::
get_required_pipes()
{
        vector_string vec;
        vec.push_back( "GBuffer" );
        vec.push_back( "CellIndices" );
        vec.push_back( "PerCellLights" );
        vec.push_back( "ShadowAtlas" );
        vec.push_back( "ShadowAtlasPCF" );
        vec.push_back( "CombinedVelocity" );
        vec.push_back( "PerCellLightsCounts" );
        return vec;
}

ApplyLightsStage::
ApplyLightsStage() :
        RenderStage()
{
}

void ApplyLightsStage::
create()
{
        _target = create_target( "ApplyLights" );
        _target->add_color_attachment( 16 );
        _target->prepare_buffer();
}

void ApplyLightsStage::
bind()
{
        rpipeline->_gbuf_stage->_ubo.bind_to( _target ); // GBuffer
        set_shader_input( new ShaderInput( "CellIndices", rpipeline->get_light_mgr()->_collect_cells_stage->_cell_index_buffer ) );
        set_shader_input( new ShaderInput( "PerCellIndices", rpipeline->get_light_mgr()->_cull_lights_stage->_per_cell_lights ) );
        set_shader_input( new ShaderInput( "ShadowAtlas", rpipeline->get_light_mgr()->_shadow_stage->get_target()->get_depth_tex() ) );
        set_shader_input( new ShaderInput( "ShadowAtlasPCF", rpipeline->get_light_mgr()->_shadow_stage->get_target()->get_depth_tex(),
                                           rpipeline->get_light_mgr()->_shadow_stage->_pcf ) );
        set_shader_input( new ShaderInput( "CombinedVelocity", rpipeline->_vel_stage->get_target()->get_color_tex() ) );
        set_shader_input( new ShaderInput( "PerCellLightsCounts", rpipeline->get_light_mgr()->_cull_lights_stage->_grouped_cell_lights_counts ) );

        set_shader_input( new ShaderInput( "AllLightsData", rpipeline->get_light_mgr()->_img_light_data ) );
        set_shader_input( new ShaderInput( "ShadowSourceData", rpipeline->get_light_mgr()->_img_source_data ) );

        bind_to_commons();
}

void ApplyLightsStage::
reload_shaders()
{
        _target->set_shader( load_shader( "shader/apply_lights.frag.glsl" ) );
}