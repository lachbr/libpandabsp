#include "cullLightsStages.h"

#include "..\..\pp_globals.h"

vector_string CullLightsStage::
get_required_inputs()
{
        vector_string vec;
        vec.push_back( "CellListBuffer" );
        return vec;
}

vector_string CullLightsStage::
get_required_pipes()
{
        vector_string vec;
        vec.push_back( "AllLightsData" );
        vec.push_back( "maxLightIndex" );
        return vec;
}

CullLightsStage::
CullLightsStage() :
        RenderStage()
{
        _cull_threads = 32;
        _num_light_classes = 4;
}

void CullLightsStage::
create()
{
        _target = create_target( "GetVisibleLights" );
        _target->set_size( LVecBase2i( 16, 16 ) );
        _target->prepare_buffer();

        _target_cull = create_target( "CullLights" );
        _target_cull->set_size( LVecBase2i( 0, 0 ) );
        _target_cull->prepare_buffer();

        _target_group = create_target( "GroupLightsByClass" );
        _target_group->set_size( LVecBase2i( 0, 0 ) );
        _target_group->prepare_buffer();

        _frustum_lights_ctr = Image::create_counter( "VisibleLightCount" );
        _frustum_lights = Image::create_buffer(
                                  "FrustumLights", MAX_LIGHT_COUNT, Texture::F_r16 );
        _per_cell_lights = Image::create_buffer(
                                   "PerCellLights", 0, Texture::F_r16i );
        _per_cell_light_counts = Image::create_buffer(
                                         "PerCellLightCounts", 0, Texture::F_r32i );
        _grouped_cell_lights = Image::create_buffer(
                                       "GroupedPerCellLights", 0, Texture::F_r16i );
        _grouped_cell_lights_counts = Image::create_buffer(
                                              "GroupedPerCellLightsCount", 0, Texture::F_r16i );

        _target->set_shader_input( new ShaderInput( "FrustumLights", _frustum_lights ) );
        _target->set_shader_input( new ShaderInput( "FrustumLightsCount", _frustum_lights_ctr ) );
        _target_cull->set_shader_input( new ShaderInput( "PerCellLightsBuffer", _per_cell_lights ) );
        _target_cull->set_shader_input( new ShaderInput( "PerCellLightCountsBuffer", _per_cell_light_counts ) );
        _target_cull->set_shader_input( new ShaderInput( "FrustumLights", _frustum_lights ) );
        _target_cull->set_shader_input( new ShaderInput( "FrustumLightsCount", _frustum_lights_ctr ) );
        _target_group->set_shader_input( new ShaderInput( "PerCellLightsBuffer", _per_cell_lights ) );
        _target_group->set_shader_input( new ShaderInput( "PerCullLightCountsBuffer", _per_cell_light_counts ) );
        _target_group->set_shader_input( new ShaderInput( "GroupedCellLightsBuffer", _grouped_cell_lights ) );
        _target_group->set_shader_input( new ShaderInput( "GroupedPerCellLightsCountBuffer", _grouped_cell_lights_counts ) );

        _target_cull->set_shader_input( new ShaderInput( "threadCount", _cull_threads ) );
        _target_group->set_shader_input( new ShaderInput( "theadCount", 1 ) );
}

void CullLightsStage::
bind()
{
        set_shader_input( new ShaderInput( "CellListBuffer", rpipeline->get_light_mgr()->_collect_cells_stage->_cell_list_buffer ) );
        set_shader_input( new ShaderInput( "AllLightsData", rpipeline->get_light_mgr()->_img_light_data ) );
        set_shader_input( new ShaderInput( "maxLightIndex", rpipeline->get_light_mgr()->_pta_max_light_index ) );

        bind_to_commons();
}

void CullLightsStage::
reload_shaders()
{
        _target_cull->set_shader( load_shader( "shader/tiled_culling.vert.glsl", "shader/cull_lights.frag.glsl" ) );
        _target_group->set_shader( load_shader( "shader/tiled_culling.vert.glsl", "shader/group_lights.frag.glsl" ) );
        _target->set_shader( load_shader( "shader/view_frustum_cell.frag.glsl" ) );
}

void CullLightsStage::
update()
{
        _frustum_lights_ctr->clear_image();
}

void CullLightsStage::
set_dimensions()
{
        int max_cells = rpipeline->get_light_mgr()->get_total_tiles();
        int num_rows_threaded = ( int )ceil( ( max_cells * _cull_threads ) / ( float )rp_lighting_culling_slice_width );
        int num_rows = ( int )ceil( max_cells / ( float )rp_lighting_culling_slice_width );
        _per_cell_lights->set_x_size( max_cells * rp_lighting_max_lights_per_cell );
        _per_cell_light_counts->set_x_size( max_cells );
        _grouped_cell_lights->set_x_size( max_cells * ( rp_lighting_max_lights_per_cell + _num_light_classes ) );
        _target_cull->set_size( LVecBase2i( rp_lighting_culling_slice_width, num_rows_threaded ) );
        _target_group->set_size( LVecBase2i( rp_lighting_culling_slice_width, num_rows ) );
}

