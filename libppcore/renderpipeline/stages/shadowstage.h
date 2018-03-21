#ifndef __SHADOW_STAGE_H__
#define __SHADOW_STAGE_H__

#include "renderStage.h"
#include <pvector.h>
#include <samplerState.h>

class ShadowStage : public RenderStage
{
public:
        static vector_string get_required_inputs();
        static vector_string get_required_pipes();

        void create() override;

        ShadowStage();

        void make_pcf_state();

        void bind();

        SamplerState _pcf;

        PT( GraphicsOutput ) get_atlas_buffer() const;

        void set_size( int size );
        int get_size() const;

        void set_shader_input( const ShaderInput *inp );

private:
        int _size;
};

#endif // __SHADOW_STAGE_H__