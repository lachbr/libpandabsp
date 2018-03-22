#include "lightManager.h"

#include "..\pp_globals.h"

LightManager::LightManager()
{
}

void LightManager::init()
{
        compute_tile_size();
        init_internal_mgr();
        init_command_queue();
        init_shadow_mgr();
        init_stages();
}

int LightManager::get_total_tiles() const
{
        return _num_tiles.get_x() * _num_tiles.get_y() * rp_lighting_culling_grid_slices;
}

LVecBase2i LightManager::get_num_tiles() const
{
        return _num_tiles;
}

LVecBase2i LightManager::get_tile_size() const
{
        return _tile_size;
}

int LightManager::get_num_lights() const
{
        return _light_mgr.get_num_lights();
}

int LightManager::get_num_shadow_sources() const
{
        return _light_mgr.get_num_shadow_sources();
}

PN_stdfloat LightManager::get_shadow_atlas_coverage() const
{
        return _light_mgr.get_shadow_manager()->get_atlas()->get_coverage() * 100.0;
}

void LightManager::reload_shaders()
{
        _cmd_queue.reload_shaders();
}

void LightManager::compute_tile_size()
{
        _tile_size = LVecBase2i( rp_lighting_culling_grid_size_x,
                                 rp_lighting_culling_grid_size_y );
        int num_tiles_x = ( int )ceil( base->get_resolution_x() / ( float )_tile_size.get_x() );
        int num_tiles_y = ( int )ceil( base->get_resolution_y() / ( float )_tile_size.get_y() );
        _num_tiles = LVecBase2i( num_tiles_x, num_tiles_y );
}

void LightManager::init_command_queue()
{
        _cmd_queue.init();
        _cmd_queue.register_input( new ShaderInput( "LightData", _img_light_data ) );
        _cmd_queue.register_input( new ShaderInput( "SourceData", _img_source_data ) );
        _light_mgr.set_command_list( _cmd_queue.get_command_list() );
}

void LightManager::init_shadow_mgr()
{
        _shadow_mgr = new ShadowManager();
        _shadow_mgr->set_max_updates( rp_shadow_max_updates );
        _shadow_mgr->set_scene( render );
        _shadow_mgr->set_tag_state_manager( &rpipeline->tag_mgr );
        _shadow_mgr->set_atlas_size( rp_atlas_size );
        _light_mgr.set_shadow_manager( _shadow_mgr );
}

void LightManager::init_stages()
{

        _flag_cells_stage = new FlagUsedCellsStage();
        rpipeline->add_stage( _flag_cells_stage );

        _collect_cells_stage = new CollectUsedCellsStage();
        rpipeline->add_stage( _collect_cells_stage );

        _cull_lights_stage = new CullLightsStage();
        rpipeline->add_stage( _cull_lights_stage );

        _apply_lights_stage = new ApplyLightsStage();
        rpipeline->add_stage( _apply_lights_stage );

        _shadow_stage = new ShadowStage();
        _shadow_stage->set_size( _shadow_mgr->get_atlas_size() );
        rpipeline->add_stage( _shadow_stage );
}

void LightManager::init_shadows()
{
        _shadow_mgr->set_atlas_graphics_output( _shadow_stage->get_atlas_buffer() );
        _shadow_mgr->init();
}

void LightManager::init_internal_mgr()
{
        _light_mgr.set_shadow_update_distance( rp_shadow_update_distance );

        const int per_light_vec4s = 4;
        _img_light_data = Image::create_buffer( "LightData", MAX_LIGHT_COUNT * per_light_vec4s, Texture::F_rgba16 );
        _img_light_data->clear_image();

        _pta_max_light_index = PTA_int::empty_array( 1 );
        _pta_max_light_index[0] = 0;

        const int per_source_vec4s = 5;
        _img_source_data = Image::create_buffer( "ShadowSourceData", MAX_SHADOW_SOURCES * per_source_vec4s, Texture::F_rgba32 );
        _img_source_data->clear_image();
}

void LightManager::update()
{
        _light_mgr.set_camera_pos( base->_window->get_camera_group().get_pos( render ) );
        _light_mgr.update();
        _shadow_mgr->update();
        _cmd_queue.process_queue();
}

void LightManager::add_light( RPLight *light )
{
        _light_mgr.add_light( light );
}

void LightManager::remove_light( RPLight *light )
{
        _light_mgr.remove_light( light );
}