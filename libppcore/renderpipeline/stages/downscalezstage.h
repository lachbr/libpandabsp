#ifndef __DOWNSCALE_Z_STAGE_H__
#define __DOWNSCALE_Z_STAGE_H__

#include "renderStage.h"

class EXPCL_PANDAPLUS DownscaleZStage : public RenderStage
{
public:

        static vector_string get_required_inputs();
        static vector_string get_required_pipes();

        DownscaleZStage();

        void create();
        void reload_shaders();

        void bind();
};

#endif // __DOWNSCALE_Z_STAGE_H__