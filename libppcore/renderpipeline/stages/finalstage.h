#ifndef __FINAL_STAGE_H__
#define __FINAL_STAGE_H__

#include "renderStage.h"

class EXPCL_PANDAPLUS FinalStage : public RenderStage
{
public:

        static vector_string get_required_inputs();
        static vector_string get_required_pipes();

        FinalStage();

        void create();
        void reload_shaders();

        void bind();

private:
        PT( RenderTarget ) _present_target;
};

#endif // __FINAL_STAGE_H__