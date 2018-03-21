#ifndef __GBUFFER_STAGE_H__
#define __GBUFFER_STAGE_H__

#include "renderStage.h"
#include "..\simpleInputBlock.h"

class EXPCL_PANDAPLUS GBufferStage : public RenderStage
{
public:

        static vector_string get_required_inputs();
        static vector_string get_required_pipes();

        GBufferStage();

        void bind();

        SimpleInputBlock _ubo;
        void make_gbuffer_ubo();
        void create();
        void set_shader_input( const ShaderInput *inp );
};

#endif // __GBUFFER_STAGE_H__