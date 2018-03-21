#ifndef __RENDER_STATE_H__
#define __RENDER_STATE_H__

#include <referenceCount.h>
#include <nodePath.h>
#include <pvector.h>
#include <pmap.h>
#include <uniqueIdAllocator.h>


#include "..\renderTarget.h"

class EXPCL_PANDAPLUS RenderStage : public ReferenceCount
{
public:
        static UniqueIdAllocator id_alloc;

        static vector_string get_required_inputs();
        static vector_string get_required_pipes();

        static bool disabled;

        RenderStage();

        PT( RenderTarget ) get_target() const;

        virtual void create();
        virtual void reload_shaders();
        virtual void update();
        virtual void set_dimensions();
        virtual void handle_window_resize();
        virtual void bind();

        void bind_to_commons();

        PT( Shader ) load_shader( const string &frag_path );
        PT( Shader ) load_shader( const string &vert_path, const string &frag_path );

        const pvector<PT( RenderTarget )> &get_targets() const;

        PT( RenderTarget ) create_target( const string &name );
        void remove_target( PT( RenderTarget ) target );

        void set_shader_input( const ShaderInput *inp );

        bool get_active() const;
        void set_active( bool active );

protected:

        bool _active;

        int _id;

        pvector<PT( RenderTarget )> _targets;
        PT( RenderTarget ) _target;

        int get_target_index( const string &name );
};

#endif // __RENDER_STATE_H__