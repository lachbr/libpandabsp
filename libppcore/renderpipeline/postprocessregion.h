#ifndef __POST_PROCESS_REGION_H__
#define __POST_PROCESS_REGION_H__

#include "..\config_pandaplus.h"
#include <nodePath.h>
#include <displayRegion.h>
#include <graphicsOutput.h>
#include <displayRegion.h>

class EXPCL_PANDAPLUS PostProcessRegion
{
public:
        PostProcessRegion();
        PostProcessRegion( PT( GraphicsOutput ) internal_buffer );

        void set_attrib( CPT( RenderAttrib ) attrib, int priority = 0 );
        void set_sort( int sort );
        void set_instance_count( int count );
        void disable_clears();
        void set_active( bool active );
        void set_clear_depth_active( bool active );
        void set_clear_depth( PN_stdfloat depth );
        void set_shader( CPT( Shader ) shader, int priority = 0 );
        void set_camera( const NodePath &camera );
        void set_clear_color_active( bool active );
        void set_clear_color( const LColor &color );

        const NodePath &get_node() const;
        const NodePath &get_tri() const;

        void set_shader_input( const ShaderInput *inp, bool override = false );

        static PostProcessRegion make( PT( GraphicsOutput ) internal_buffer );

private:
        PT( GraphicsOutput ) _buffer;
        PT( DisplayRegion ) _region;

        void make_fullscreen_tri();
        void make_fullscreen_cam();
        NodePath _tri;
        NodePath _cam;
        NodePath _node;
};

#endif // __POST_PROCESS_REGION_H__