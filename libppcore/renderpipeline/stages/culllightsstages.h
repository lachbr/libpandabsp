#ifndef __CULL_LIGHTS_STAGE_H__
#define __CULL_LIGHTS_STAGE_H__

#include "renderStage.h"
#include "..\image.h"

class EXPCL_PANDAPLUS CullLightsStage : public RenderStage
{
public:

        static vector_string get_required_inputs();
        static vector_string get_required_pipes();

        CullLightsStage();

        void create();
        void reload_shaders();
        void update();
        void set_dimensions();

        PT( Image ) _frustum_lights;
        PT( Image ) _per_cell_lights;
        PT( Image ) _per_cell_light_counts;
        PT( Image ) _frustum_lights_ctr;
        PT( Image ) _grouped_cell_lights;
        PT( Image ) _grouped_cell_lights_counts;

        void bind();

private:


        PT( RenderTarget ) _target_cull;
        PT( RenderTarget ) _target_group;

        int _cull_threads;
        int _num_light_classes;
};

#endif // __CULL_LIGHTS_STAGE_H__