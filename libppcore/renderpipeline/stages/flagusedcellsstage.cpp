#include "flagUsedCellsStage.h"
#include "..\..\config_pandaplus.h"

#include "..\..\pp_globals.h"

vector_string FlagUsedCellsStage::get_required_inputs()
{
        vector_string vec;
        return vec;
}

vector_string FlagUsedCellsStage::get_required_pipes()
{
        vector_string vec;
        vec.push_back( "GBuffer" );
        return vec;
}

FlagUsedCellsStage::FlagUsedCellsStage() :
        RenderStage()
{
}

void FlagUsedCellsStage::create()
{
        _target = create_target( "FlagUsedCells" );
        _target->prepare_buffer();

        _cell_grid_flags = Image::create_2d_array(
                                   "CellGridFlags", 0, 0,
                                   rp_lighting_culling_grid_slices, Texture::F_r8i );
}

void FlagUsedCellsStage::bind()
{
        rpipeline->_gbuf_stage->_ubo.bind_to( _target );
        bind_to_commons();
}

void FlagUsedCellsStage::update()
{
        _cell_grid_flags->clear_image();
}

void FlagUsedCellsStage::set_dimensions()
{
        LVecBase2i tile_amount = rpipeline->get_light_mgr()->get_num_tiles();
        _cell_grid_flags->set_x_size( tile_amount.get_x() );
        _cell_grid_flags->set_y_size( tile_amount.get_y() );
}

void FlagUsedCellsStage::reload_shaders()
{
        _target->set_shader( load_shader( "shader\flag_used_cells_stage.frag.glsl" ) );
}