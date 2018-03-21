#ifndef __RENDER_TARGET_H__
#define __RENDER_TARGET_H__

#include <referenceCount.h>
#include <pointerTo.h>
#include <lvector4.h>
#include <graphicsEngine.h>
#include <graphicsWindow.h>
#include <graphicsOutput.h>

#include "postProcessRegion.h"

class RenderTarget : public ReferenceCount
{
public:
        static int current_sort;
        static pvector<PT( RenderTarget )> registered_targets;
        static bool use_r11g11b10;
        static int num_allocated_buffers;

        RenderTarget();
        RenderTarget( const string &name );
        string get_name() const;
        void set_active( bool active );
        bool get_active() const;
        void consider_resize();

        void add_color_attachment( int bits = 8, bool alpha = false );
        void add_color_attachment( LVecBase4i &bits, bool alpha = false );
        void add_depth_attachment( int bits = 32 );
        void add_aux_attachment( int bits = 8 );
        void add_aux_attachments( int bits = 8, int count = 1 );

        void prepare_render( NodePath &camera );
        void prepare_buffer();
        void present_on_screen();
        void remove();
        void set_clear_color( const LColor &color );

        void set_shader_input( const ShaderInput *inp );
        void set_shader( const Shader *shad );

        PT( GraphicsOutput ) get_internal_buffer() const;

        PostProcessRegion _source_region_def;

        void set_size( LVecBase2i &size );
        LVecBase2i get_size() const;

        PT( Texture ) get_color_tex() const;
        PT( Texture ) get_depth_tex() const;

        PT( Texture ) get_aux_tex( int n ) const;

private:

        struct PropertiesData
        {
                WindowProperties winprop;
                FrameBufferProperties bufprop;
        };


        PT( DisplayRegion ) _source_region;

        int get_max_color_bits() const;

        bool create_buffer();
        void compute_size_from_constraint();
        void setup_textures();
        PropertiesData make_properties();
        bool create();

        PT( Texture ) _color_target;
        PT( Texture ) _depth_target;

        pvector<PT( Texture )> _aux_texs;

        string _name;
        pvector<PT( RenderTarget )> _targets;
        LVecBase4i _color_bits;
        int _aux_bits;
        int _aux_count;
        int _depth_bits;
        LVecBase2i _size;
        LVecBase2i _size_constraint;
        int _sort;

        bool _active;

        bool _support_transparency;
        bool _create_default_region;

        PT( GraphicsEngine ) _engine;
        PT( GraphicsWindow ) _source_window;
        PT( GraphicsOutput ) _internal_buffer;
};

#endif // __RENDER_TARGET_H__