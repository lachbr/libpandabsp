#ifndef __LIGHT_MANAGER_H__
#define __LIGHT_MANAGER_H__

#include "shadowManager.h"
#include "internalLightManager.h"
#include "..\config_pandaplus.h"
#include "image.h"

#include "stages\shadowStage.h"
#include "stages\flagUsedCellsStage.h"
#include "stages\collectUsedCellsStage.h"
#include "stages\cullLightsStages.h"
#include "stages\applyLightsStage.h"

#include "gpuCommandQueue.h"

#include <pta_int.h>

class EXPCL_PANDAPLUS LightManager
{
public:
        LightManager();

        void init();

        void add_light( RPLight *light );
        void remove_light( RPLight *light );

        PN_stdfloat get_shadow_atlas_coverage() const;
        int get_num_shadow_sources() const;
        int get_num_lights() const;
        int get_total_tiles() const;

        void init_shadows();

        void init_stages();

        void reload_shaders();

        void update();

        LVecBase2i get_num_tiles() const;
        LVecBase2i get_tile_size() const;

        PT( ShadowStage ) _shadow_stage;
        PT( FlagUsedCellsStage ) _flag_cells_stage;
        PT( CollectUsedCellsStage ) _collect_cells_stage;
        PT( CullLightsStage ) _cull_lights_stage;
        PT( ApplyLightsStage ) _apply_lights_stage;

        PT( Image ) _img_light_data;
        PT( Image ) _img_source_data;

        PTA_int _pta_max_light_index;

private:
        PT( ShadowManager ) _shadow_mgr;
        InternalLightManager _light_mgr;



        GPUCommandQueue _cmd_queue;

        void init_internal_mgr();
        void init_shadow_mgr();
        void init_command_queue();
        void compute_tile_size();

        LVecBase2i _tile_size;
        LVecBase2i _num_tiles;




};

#endif // __LIGHT_MANAGER_H__