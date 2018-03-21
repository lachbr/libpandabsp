#ifndef __RENDER_PIPELINE_H__
#define __RENDER_PIPELINE_H__

#include "..\config_pandaplus.h"
#include "lightManager.h"
#include "shadowManager.h"
#include "tagStateManager.h"
#include "stages\stageManager.h"

#include "stages\ambientStage.h"
#include "stages\gBufferStage.h"
#include "stages\finalStage.h"
#include "stages\downscaleZStage.h"
#include "stages\combineVelocityStage.h"

#include <genericAsyncTask.h>

class EXPCL_PANDAPLUS RenderPipeline
{
public:

        RenderPipeline();

        LightManager _light_mgr;
        StageManager _stage_mgr;

        void add_light( RPLight *light );
        void remove_light( RPLight *light );

        void reload_shaders();

        LightManager *get_light_mgr();

        void add_stage( RenderStage *stage );
        void remove_stage( RenderStage *stage );

        TagStateManager tag_mgr;

        void init();

        PT( AmbientStage ) _amb_stage;
        PT( GBufferStage ) _gbuf_stage;
        PT( FinalStage ) _final_stage;
        PT( DownscaleZStage ) _dz_stage;
        PT( CombineVelocityStage ) _vel_stage;

        PT( Texture ) _def_envmap;
        PT( Texture ) _pref_brdf;
        PT( Texture ) _pref_metal_brdf;
        PT( Texture ) _pref_coat_brdf;

        PTA_LMatrix4 _view_mat_z_up;
        PTA_LMatrix4 _view_mat_billboard;
        PTA_LVecBase3 _camera_pos;
        PTA_LMatrix4 _last_view_proj_mat_no_jitter;
        PTA_LMatrix4 _last_inv_view_proj_mat_no_jitter;
        PTA_LMatrix4 _proj_mat;
        PTA_LMatrix4 _inv_proj_mat;
        PTA_LMatrix4 _view_proj_mat_no_jitter;
        PTA_float _frame_delta;
        PTA_float _smooth_frame_delta;
        PTA_float _frame_time;
        PTA_LVecBase2f _current_film_offset;
        PTA_int _frame_index;
        PTA_LMatrix4 _vs_frustum_directions;
        PTA_LMatrix4 _ws_frustum_directions;
        PTA_LVecBase2i _screen_size;
        PTA_LVecBase2i _native_screen_size;
        PTA_LVecBase2i _lc_tile_count;

private:
        void create_managers();
        void adjust_camera_settings();
        void initialize_managers();
        void init_bindings();
        void init_common_stages();

        static void handle_window_event( const Event *e, void *data );

        void manager_tick();
        static AsyncTask::DoneStatus manager_tick_task( GenericAsyncTask *task, void *data );

        void inp_and_st_tick();
        static AsyncTask::DoneStatus inp_and_st_tick_task( GenericAsyncTask *task, void *data );

        PT( GenericAsyncTask ) _manager_update_task;
        PT( GenericAsyncTask ) _inp_and_st_task;


};

#endif // __RENDER_PIPELINE_H__