#ifndef __AMBIENT_STAGE_H__
#define __AMBIENT_STAGE_H__

#include "renderStage.h"

class EXPCL_PANDAPLUS AmbientStage : public RenderStage
{
public:

        static vector_string get_required_inputs();
        static vector_string get_required_pipes();

        AmbientStage();

        void create();
        void reload_shaders();

        void bind();
};

#endif // __AMBIENT_STAGE_H__