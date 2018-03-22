#include "collectUsedCellsStage.h"
#include "..\..\config_pandaplus.h"

#include "..\..\pp_globals.h"

vector_string CollectUsedCellsStage::get_required_inputs()
{
        vector_string vec;
        return vec;
}

vector_string CollectUsedCellsStage::get_required_pipes()
{
        vector_string vec;
        vec.push_back( "FlaggedCells" );
        return vec;
}

CollectUsedCellsStage::CollectUsedCellsStage() :
        RenderStage()
{
}

void CollectUsedCellsStage::create()
{
        _target = create_target( "CollectUsedCells" );
        _target->set_size( LVecBase2i( 0, 0 ) );
        _target->prepare_buffer();

        _cell_list_buffer = Image::create_buffer( "CellList", 0, Texture::F_r32i );
        _cell_index_buffer = Image::create_2d_array( "CellIndices", 0, 0, 0, Texture::F_r32i );

        _target->set_shader_input( new ShaderInput( "CellListBuffer", _cell_list_buffer ) );
        _target->set_shader_input( new ShaderInput( "CellListIndices", _cell_index_buffer ) );
}

void CollectUsedCellsStage::bind()
{
        set_shader_input( new ShaderInput( "FlaggedCells", rpipeline->get_light_mgr()->_flag_cells_stage->_cell_grid_flags ) );

        bind_to_commons();
}

void CollectUsedCellsStage::update()
{
        _cell_list_buffer->clear_image();
        _cell_index_buffer->clear_image();
}

void CollectUsedCellsStage::set_dimensions()
{
        LVecBase2i tile_amt = rpipeline->get_light_mgr()->get_num_tiles();
        int num_slices = rp_lighting_culling_grid_slices;
        int max_cells = tile_amt.get_x() * tile_amt.get_y() * num_slices;

        _cell_list_buffer->set_x_size( 1 + max_cells );
        _cell_index_buffer->set_x_size( tile_amt.get_x() );
        _cell_index_buffer->set_y_size( tile_amt.get_y() );
        _cell_index_buffer->set_z_size( num_slices );

        _cell_list_buffer->clear_image();
        _cell_index_buffer->clear_image();

        _target->set_size( tile_amt );
}

void CollectUsedCellsStage::reload_shaders()
{
        _target->set_shader( load_shader( "shader/collect_used_cells.frag.glsl" ) );
}