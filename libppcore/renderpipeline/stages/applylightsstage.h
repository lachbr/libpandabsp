#ifndef __APPLY_LIGHTS_STAGE_H__
#define __APPLY_LIGHTS_STAGE_H__

#include "renderStage.h"

class EXPCL_PANDAPLUS ApplyLightsStage : public RenderStage
{
public:

        static vector_string get_required_inputs();
        static vector_string get_required_pipes();

        ApplyLightsStage();

        void create();
        void reload_shaders();

        void bind();
};

#endif // __APPLY_LIGHTS_STAGE_H__