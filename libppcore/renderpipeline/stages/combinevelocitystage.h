#ifndef __COMBINE_VELOCITY_STAGE_H__
#define __COMBINE_VELOCITY_STAGE_H__

#include "renderStage.h"

class EXPCL_PANDAPLUS CombineVelocityStage : public RenderStage
{
public:

        static vector_string get_required_inputs();
        static vector_string get_required_pipes();

        CombineVelocityStage();

        void create();
        void reload_shaders();

        void bind();
};

#endif // __COMBINE_VELOCTIY_STAGE_H__