#ifndef FILTER_MANAGER_H
#define FILTER_MANAGER_H

#include "config_pandaplus.h"

#include <graphicsOutput.h>
#include <nodePath.h>

class EXPCL_PANDAPLUS FilterManager
{
public:
        struct ClearData
        {
                bool active;
                LColor value;
        };
        typedef epvector<ClearData> ClearDataVec;

        typedef pmap<string, PT( Texture )> TextureMap;
        typedef pvector<PT( Texture )> TextureVec;
        typedef pvector<PT( GraphicsOutput )> BufferVec;
        typedef epvector<LVector3> SizeVec;

        FilterManager( PT( GraphicsOutput ) win, NodePath &cam, int force_x = 0, int force_y = 0 );

        void get_both_clears();
        ClearDataVec get_clears( GraphicsOutput *region ) const;
        ClearDataVec get_clears( DisplayRegion *region ) const;
        void set_clears( PT( GraphicsOutput ) region, ClearDataVec &clears );
        void set_clears( PT( DisplayRegion ) region, ClearDataVec &clears );
        ClearDataVec set_stacked_clears( PT( GraphicsOutput ) region, ClearDataVec &clears0, ClearDataVec &clears1 );

        bool is_fullscreen() const;

        LVector2f get_scaled_size( PN_stdfloat mul, PN_stdfloat div, PN_stdfloat align ) const;

        NodePath render_scene_into( PT( Texture ) depth_tex = nullptr, PT( Texture ) color_tex = nullptr,
                                    PT( Texture ) aux_tex = nullptr, int aux_bits = 0, TextureMap &textures = TextureMap() );

        NodePath render_quad_into( PN_stdfloat mul = 1, PN_stdfloat div = 1, PN_stdfloat align = 1, PT( Texture ) depth_tex = nullptr,
                                   PT( Texture ) color_tex = nullptr, PT( Texture ) aux_tex0 = nullptr, PT( Texture ) aux_tex1 = nullptr );

        PT( GraphicsOutput ) create_buffer( const string &name, int x_size, int y_size, TextureVec &tex_group, int depth_bits = 1 );

        void resize_buffers();

        void cleanup();

        NodePath &get_cam();

private:
        static void window_event( const Event *e, void *data );

private:
        ClearDataVec _r_clears;
        ClearDataVec _w_clears;

        BufferVec _buffers;
        SizeVec _sizes;

        PT( GraphicsOutput ) _win;
        NodePath _cam;
        int _force_x;
        int _force_y;
        PT( GraphicsEngine ) _engine;
        PT( DisplayRegion ) _region;
        CPT( RenderState ) _cam_init;
        CPT( RenderState ) _cam_state;
        int _next_sort;
        int _base_x;
        int _base_y;
};

#endif // FILTER_MANAGER_H