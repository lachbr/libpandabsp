#include "renderPipeline.h"
#include "..\pp_globals.h"
#include <texturePool.h>

RenderPipeline::RenderPipeline()
{
        // Initialize all of the PTAs.
        _view_mat_z_up                    = PTA_LMatrix4::empty_array( 1 );
        _view_mat_billboard               = PTA_LMatrix4::empty_array( 1 );
        _camera_pos                       = PTA_LVecBase3::empty_array( 1 );
        _last_view_proj_mat_no_jitter     = PTA_LMatrix4::empty_array( 1 );
        _last_inv_view_proj_mat_no_jitter = PTA_LMatrix4::empty_array( 1 );
        _proj_mat                         = PTA_LMatrix4::empty_array( 1 );
        _inv_proj_mat                     = PTA_LMatrix4::empty_array( 1 );
        _view_proj_mat_no_jitter          = PTA_LMatrix4::empty_array( 1 );
        _frame_delta                      = PTA_float::empty_array( 1 );
        _smooth_frame_delta               = PTA_float::empty_array( 1 );
        _frame_time                       = PTA_float::empty_array( 1 );
        _current_film_offset              = PTA_LVecBase2f::empty_array( 1 );
        _frame_index                      = PTA_int::empty_array( 1 );
        _vs_frustum_directions            = PTA_LMatrix4::empty_array( 1 );
        _ws_frustum_directions            = PTA_LMatrix4::empty_array( 1 );
        _screen_size                      = PTA_LVecBase2i::empty_array( 1 );
        _native_screen_size               = PTA_LVecBase2i::empty_array( 1 );
        _lc_tile_count                    = PTA_LVecBase2i::empty_array( 1 );
}

void RenderPipeline::initialize_managers()
{
        _stage_mgr.setup();
        _stage_mgr.reload_shaders();
        _light_mgr.reload_shaders();
        init_bindings();
        _light_mgr.init_shadows();
}

void RenderPipeline::manager_tick()
{
        _light_mgr.update();
}

AsyncTask::DoneStatus RenderPipeline::manager_tick_task( GenericAsyncTask *task, void *data )
{
        ( ( RenderPipeline * )data )->manager_tick();
        return AsyncTask::DS_cont;
}

void RenderPipeline::inp_and_st_tick()
{
        // Update all of the MainSceneData inputs:
        LMatrix4 view_mat = render.get_transform( base->_window->get_camera_group() )->get_mat();
        LMatrix4 zup_conversion = LMatrix4::convert_mat( CS_zup_right, CS_zup_right );
        _view_mat_z_up[0] = view_mat * zup_conversion;

        LMatrix4 vmb = LMatrix4( view_mat );
        vmb.set_row( 0, LVector3( 1, 0, 0 ) );
        vmb.set_row( 1, LVector3( 0, 1, 0 ) );
        vmb.set_row( 2, LVector3( 0, 0, 1 ) );
        _view_mat_billboard[0] = vmb;

        _camera_pos[0] = base->_window->get_camera_group().get_pos( render );

        LMatrix4 curr_vp = _view_proj_mat_no_jitter[0];
        _last_view_proj_mat_no_jitter[0] = curr_vp;
        curr_vp = LMatrix4( curr_vp );
        curr_vp.invert_in_place();
        LMatrix4 curr_inv_vp = curr_vp;
        _last_inv_view_proj_mat_no_jitter[0] = curr_inv_vp;

        LMatrix4 proj_mat = LMatrix4( base->_window->get_camera( 0 )->get_lens()->get_projection_mat() );
        LMatrix4 proj_mat_zup = LMatrix4::convert_mat( CS_yup_right, CS_zup_right ) * proj_mat;
        _proj_mat[0] = proj_mat_zup;

        _inv_proj_mat[0] = invert( proj_mat_zup );

        proj_mat.set_cell( 1, 0, 0.0 );
        proj_mat.set_cell( 1, 1, 0.0 );
        _view_proj_mat_no_jitter[0] = view_mat * proj_mat;

        _frame_delta[0] = global_clock->get_dt();
        _smooth_frame_delta[0] = 1.0 / max( 1e-5, global_clock->get_average_frame_rate() );
        _frame_time[0] = global_clock->get_frame_time();

        _current_film_offset[0] = base->_window->get_camera( 0 )->get_lens()->get_film_offset();
        _frame_index[0] = global_clock->get_frame_count();

        LMatrix4 ws_frustum_directions;
        LMatrix4 vs_frustum_directions;
        LMatrix4 inv_proj_mat = base->_window->get_camera( 0 )->get_lens()->get_projection_mat_inv();
        LMatrix4 view_mat_inv = LMatrix4( view_mat );
        view_mat_inv.invert_in_place();

        vector<vector<int>> points;
        vector<int> one;
        one.push_back( -1 );
        one.push_back( -1 );
        vector<int> two;
        two.push_back( 1 );
        two.push_back( -1 );
        vector<int> three;
        three.push_back( -1 );
        three.push_back( 1 );
        vector<int> four;
        four.push_back( 1 );
        four.push_back( 1 );
        points.push_back( one );
        points.push_back( two );
        points.push_back( three );
        points.push_back( four );

        for ( size_t i = 0; i < points.size(); i++ )
        {
                vector<int> vec = points[i];
                LVecBase4 result = inv_proj_mat.xform( LVector4( vec[0], vec[1], 1.0, 1.0 ) );
                LVecBase3 vs_dir = ( zup_conversion.xform( result ) ).get_xyz().normalized();
                vs_frustum_directions.set_row( i, LVector4( vs_dir, 1 ) );
                LVecBase4 ws_dir = view_mat_inv.xform( LVector4( result.get_xyz(), 0.0 ) );
                ws_frustum_directions.set_row( i, ws_dir );
        }

        _vs_frustum_directions[0] = vs_frustum_directions;
        _ws_frustum_directions[0] = ws_frustum_directions;

        _screen_size[0] = LVecBase2i( base->get_resolution_x(), base->get_resolution_y() );
        _native_screen_size[0] = _screen_size[0];
        _lc_tile_count[0] = _light_mgr.get_num_tiles();
        /////////////////////////////////////////////////////////////////////////////////////

        _stage_mgr.update();
}

AsyncTask::DoneStatus RenderPipeline::inp_and_st_tick_task( GenericAsyncTask *task, void *data )
{
        ( ( RenderPipeline * )data )->inp_and_st_tick();
        return AsyncTask::DS_cont;
}

void RenderPipeline::init_bindings()
{
        _manager_update_task = new GenericAsyncTask( "RP_UpdateManagers", manager_tick_task, this );
        _manager_update_task->set_sort( 10 );
        task_mgr->add( _manager_update_task );

        _inp_and_st_task = new GenericAsyncTask( "RP_UpdateStages+Inps", inp_and_st_tick_task, this );
        _inp_and_st_task->set_sort( 18 );
        task_mgr->add( _inp_and_st_task );

        base->_framework.get_event_handler().add_hook( "window-event", handle_window_event, this );
}

void RenderPipeline::handle_window_event( const Event *e, void *data )
{
        LVecBase2i win_dims = LVecBase2i( base->get_resolution_x(), base->get_resolution_y() );
}

void RenderPipeline::init()
{
        _def_envmap = TexturePool::load_cube_map( "data/default_cubemap/cubemap.txo", true );
        _def_envmap->set_minfilter( SamplerState::FT_linear_mipmap_linear );
        _def_envmap->set_magfilter( SamplerState::FT_linear );
        _def_envmap->set_wrap_u( SamplerState::WM_repeat );
        _def_envmap->set_wrap_v( SamplerState::WM_repeat );
        _def_envmap->set_wrap_w( SamplerState::WM_repeat );

        _pref_brdf = TexturePool::load_3d_texture( "data/environment_brdf/slices/env_brdf_#.png" );
        _pref_brdf->set_minfilter( SamplerState::FT_linear );
        _pref_brdf->set_magfilter( SamplerState::FT_linear );
        _pref_brdf->set_wrap_u( SamplerState::WM_clamp );
        _pref_brdf->set_wrap_v( SamplerState::WM_clamp );
        _pref_brdf->set_wrap_w( SamplerState::WM_clamp );
        _pref_brdf->set_anisotropic_degree( 0 );

        _pref_metal_brdf = TexturePool::load_texture( "data/environment_brdf/slices_metal/env_brdf.png" );
        _pref_metal_brdf->set_minfilter( SamplerState::FT_linear );
        _pref_metal_brdf->set_magfilter( SamplerState::FT_linear );
        _pref_metal_brdf->set_wrap_u( SamplerState::WM_clamp );
        _pref_metal_brdf->set_wrap_v( SamplerState::WM_clamp );
        _pref_metal_brdf->set_wrap_w( SamplerState::WM_clamp );
        _pref_metal_brdf->set_anisotropic_degree( 0 );

        _pref_coat_brdf = TexturePool::load_texture( "data/environment_brdf/slices_coat/env_brdf.png" );
        _pref_coat_brdf->set_minfilter( SamplerState::FT_linear );
        _pref_coat_brdf->set_magfilter( SamplerState::FT_linear );
        _pref_coat_brdf->set_wrap_u( SamplerState::WM_clamp );
        _pref_coat_brdf->set_wrap_v( SamplerState::WM_clamp );
        _pref_coat_brdf->set_wrap_w( SamplerState::WM_clamp );
        _pref_coat_brdf->set_anisotropic_degree( 0 );

        _light_mgr.init();

        adjust_camera_settings();
        create_managers();
        initialize_managers();
}

LightManager *RenderPipeline::get_light_mgr()
{
        return &_light_mgr;
}

void RenderPipeline::add_stage( RenderStage *stage )
{
        _stage_mgr.add_stage( stage );
}

void RenderPipeline::remove_stage( RenderStage *stage )
{
        _stage_mgr.remove_stage( stage );
}

void RenderPipeline::create_managers()
{
        tag_mgr = TagStateManager( base->_window->get_camera_group() );
        init_common_stages();
}

void RenderPipeline::reload_shaders()
{
        tag_mgr.cleanup_states();
        _stage_mgr.reload_shaders();
        _light_mgr.reload_shaders();
}

void RenderPipeline::adjust_camera_settings()
{
        base->_window->get_camera( 0 )->get_lens()->set_near_far( 0.1, 70000 );
        base->_window->get_camera( 0 )->get_lens()->set_fov( 40 );
}

void RenderPipeline::add_light( RPLight *light )
{
        _light_mgr.add_light( light );
}

void RenderPipeline::remove_light( RPLight *light )
{
        _light_mgr.remove_light( light );
}

void RenderPipeline::init_common_stages()
{
        _amb_stage = new AmbientStage();
        add_stage( _amb_stage );

        _gbuf_stage = new GBufferStage();
        add_stage( _gbuf_stage );

        _final_stage = new FinalStage();
        add_stage( _final_stage );

        _dz_stage = new DownscaleZStage();
        add_stage( _dz_stage );

        _vel_stage = new CombineVelocityStage();
        add_stage( _vel_stage );
}