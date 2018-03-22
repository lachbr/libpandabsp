#include "filtermanager.h"
#include "pp_globals.h"

#include <graphicsEngine.h>
#include <cardMaker.h>
#include <auxBitplaneAttrib.h>
#include <orthographicLens.h>

FilterManager::FilterManager( PT( GraphicsOutput ) win, NodePath &cam, int force_x, int force_y ) :
        _win( win ),
        _cam( cam ),
        _force_x( force_x ),
        _force_y( force_y ),
        _base_x( 0 ),
        _base_y( 0 ),
        _engine( nullptr ),
        _region( nullptr ),
        _cam_init( nullptr ),
        _cam_state( nullptr ),
        _next_sort( 0 )
{

        int num_regions = win->get_num_display_regions();
        for ( int i = 0; i < num_regions; i++ )
        {
                PT( DisplayRegion ) dr = win->get_display_region( i );
                NodePath dr_cam = dr->get_camera();
                if ( dr_cam == cam )
                {
                        _region = dr;
                        break;
                }
        }

        if ( _region == nullptr )
        {
                cerr << "Could not find appropriate DisplayRegion to filter" << endl;
                return;
        }

        _engine = win->get_gsg()->get_engine();
        _cam_init = DCAST( Camera, cam.node() )->get_initial_state();
        _cam_state = _cam_init;
        get_both_clears();
        _next_sort = win->get_sort() - 1000;

        g_base->define_key( "g_window-event", "filterManager handle g_window event", window_event, this );
}

void FilterManager::get_both_clears()
{
        _w_clears = get_clears( _win );
        _r_clears = get_clears( _region );
}

FilterManager::ClearDataVec FilterManager::get_clears( GraphicsOutput *region ) const
{
        ClearDataVec clears;

        for ( int i = 0; i < GraphicsOutput::RTP_COUNT; i++ )
        {
                ClearData clear;
                clear.active = region->get_clear_active( i );
                clear.value = region->get_clear_value( i );
                clears.push_back( clear );
        }

        return clears;
}

FilterManager::ClearDataVec FilterManager::get_clears( DisplayRegion *region ) const
{
        ClearDataVec clears;

        for ( int i = 0; i < GraphicsOutput::RTP_COUNT; i++ )
        {
                ClearData clear;
                clear.active = region->get_clear_active( i );
                clear.value = region->get_clear_value( i );
                clears.push_back( clear );
        }

        return clears;
}

void FilterManager::set_clears( PT( GraphicsOutput ) region, ClearDataVec &clears )
{
        for ( int i = 0; i < GraphicsOutput::RTP_COUNT; i++ )
        {
                ClearData cdata = clears[i];
                region->set_clear_active( i, cdata.active );
                region->set_clear_value( i, cdata.value );
        }
}

void FilterManager::set_clears( PT( DisplayRegion ) region, ClearDataVec &clears )
{
        for ( int i = 0; i < GraphicsOutput::RTP_COUNT; i++ )
        {
                ClearData cdata = clears[i];
                region->set_clear_active( i, cdata.active );
                region->set_clear_value( i, cdata.value );
        }
}

FilterManager::ClearDataVec FilterManager::set_stacked_clears( PT( GraphicsOutput ) region, ClearDataVec &clears0,
                                                               ClearDataVec &clears1 )
{
        ClearDataVec clears;

        for ( int i = 0; i < GraphicsOutput::RTP_COUNT; i++ )
        {
                ClearData cdata = clears0[i];
                if ( !cdata.active )
                {
                        cdata = clears1[i];
                }
                region->set_clear_active( i, cdata.active );
                region->set_clear_value( i, cdata.value );
        }

        return clears;
}

bool FilterManager::is_fullscreen() const
{
        return ( _region->get_left() == 0.0 &&
                 _region->get_right() == 1.0 &&
                 _region->get_bottom() == 0.0 &&
                 _region->get_top() == 1.0 );
}

LVector2f FilterManager::get_scaled_size( PN_stdfloat mul, PN_stdfloat div, PN_stdfloat align ) const
{
        int win_x = _force_x;
        int win_y = _force_y;
        if ( win_x == 0 )
        {
                win_x = _win->get_x_size();
        }
        if ( win_y == 0 )
        {
                win_y = _win->get_y_size();
        }

        if ( div != -1 )
        {
                win_x = ( ( win_x + align - 1 ) / align ) * align;
                win_y = ( ( win_y + align - 1 ) / align ) * align;
                win_x /= div;
                win_y /= div;
        }

        if ( mul != -1 )
        {
                win_x *= mul;
                win_y *= mul;
        }

        return LVector2f( win_x, win_y );
}

NodePath FilterManager::render_scene_into( PT( Texture ) depth_tex, PT( Texture ) color_tex,
                                           PT( Texture ) aux_tex, int aux_bits, TextureMap &textures )
{

        PT( Texture ) aux_tex0 = nullptr;
        PT( Texture ) aux_tex1 = nullptr;

        if ( textures.size() > (size_t)0 )
        {
                if ( textures.find( "color" ) != textures.end() )
                {
                        color_tex = textures["color"];
                }
                if ( textures.find( "depth" ) != textures.end() )
                {
                        depth_tex = textures["depth"];
                }
                if ( textures.find( "aux" ) != textures.end() )
                {
                        aux_tex = textures["aux"];
                }
                if ( textures.find( "aux0" ) != textures.end() )
                {
                        aux_tex0 = textures["aux0"];
                }
                if ( textures.find( "aux1" ) != textures.end() )
                {
                        aux_tex1 = textures["aux1"];
                }
        }
        else
        {
                aux_tex0 = aux_tex;
        }

        if ( color_tex == nullptr )
        {
                color_tex = new Texture( "filter-base-color" );
                color_tex->set_wrap_u( SamplerState::WM_clamp );
                color_tex->set_wrap_v( SamplerState::WM_clamp );
        }

        TextureVec tex_group;
        tex_group.push_back( depth_tex );
        tex_group.push_back( color_tex );
        tex_group.push_back( aux_tex0 );
        tex_group.push_back( aux_tex1 );

        LVector2f size = get_scaled_size( 1, 1, 1 );

        PT( GraphicsOutput ) buffer = create_buffer( "filter-base", size[0], size[1], tex_group );

        if ( buffer == nullptr )
        {
                return NodePath();
        }

        CardMaker cm( "filter-base-quad" );
        cm.set_frame_fullscreen_quad();
        NodePath quad( cm.generate() );
        quad.set_depth_test( false );
        quad.set_depth_write( false );
        quad.set_texture( color_tex );
        quad.set_color( 1.0, 0.5, 0.5, 1.0 );

        NodePath cs( "dummy" );
        cs.set_state( _cam_state );
        if ( aux_bits > 0 )
        {
                cs.set_attrib( AuxBitplaneAttrib::make( aux_bits ) );
        }
        DCAST( Camera, _cam.node() )->set_initial_state( cs.get_state() );

        PT( Camera ) quad_cam_node = new Camera( "filter-quad-cam" );
        PT( OrthographicLens ) lens = new OrthographicLens;
        lens->set_film_size( 2, 2 );
        lens->set_film_offset( 0, 0 );
        lens->set_near_far( -1000, 1000 );
        quad_cam_node->set_lens( lens );
        NodePath quad_cam( quad.attach_new_node( quad_cam_node ) );

        _region->set_camera( quad_cam );

        set_stacked_clears( buffer, _r_clears, _w_clears );

        if ( aux_tex0 != nullptr )
        {
                buffer->set_clear_active( GraphicsOutput::RTP_aux_rgba_0, true );
                buffer->set_clear_value( GraphicsOutput::RTP_aux_rgba_0, LColor( 0.5, 0.5, 1.0, 0.0 ) );
        }
        if ( aux_tex1 != nullptr )
        {
                buffer->set_clear_active( GraphicsOutput::RTP_aux_rgba_1, true );
        }

        _region->disable_clears();

        if ( is_fullscreen() )
        {
                _win->disable_clears();
        }

        PT( DisplayRegion ) dr = buffer->make_display_region();
        dr->disable_clears();
        dr->set_camera( _cam );
        dr->set_active( true );

        _buffers.push_back( buffer );
        _sizes.push_back( LVector3( 1, 1, 1 ) );

        return quad;
}

NodePath FilterManager::render_quad_into( PN_stdfloat mul, PN_stdfloat div, PN_stdfloat align, PT( Texture ) depth_tex,
                                          PT( Texture ) color_tex, PT( Texture ) aux_tex0, PT( Texture ) aux_tex1 )
{
        TextureVec tex_group;
        tex_group.push_back( depth_tex );
        tex_group.push_back( color_tex );
        tex_group.push_back( aux_tex0 );
        tex_group.push_back( aux_tex1 );

        LVector2f size = get_scaled_size( mul, div, align );

        bool depth_bits = depth_tex != nullptr;

        PT( GraphicsOutput ) buffer = create_buffer( "filter-stage", size[0], size[1], tex_group, depth_bits );

        if ( buffer == nullptr )
        {
                return NodePath();
        }

        CardMaker cm( "filter-stage-quad" );
        cm.set_frame_fullscreen_quad();
        NodePath quad( cm.generate() );
        quad.set_depth_test( false );
        quad.set_depth_write( false );
        quad.set_color( 1.0, 0.5, 0.5, 1.0 );

        PT( Camera ) quad_cam_node = new Camera( "filter-quad-cam" );
        PT( OrthographicLens ) lens = new OrthographicLens;
        lens->set_film_size( 2, 2 );
        lens->set_film_offset( 0, 0 );
        lens->set_near_far( -1000, 1000 );
        quad_cam_node->set_lens( lens );
        NodePath quad_cam( quad.attach_new_node( quad_cam_node ) );

        PT( DisplayRegion ) dr = buffer->make_display_region( LVector4( 0, 1, 0, 1 ) );
        dr->disable_clears();
        dr->set_camera( quad_cam );
        dr->set_active( true );
        dr->set_scissor_enabled( false );

        buffer->set_clear_color( LColor( 0, 0, 0, 1 ) );
        buffer->set_clear_color_active( true );

        _buffers.push_back( buffer );
        _sizes.push_back( LVector3( mul, div, align ) );

        return quad;
}

PT( GraphicsOutput ) FilterManager::create_buffer( const string &name, int x_size, int y_size, TextureVec &tex_group, int depth_bits )
{
        WindowProperties winprops;
        winprops.set_size( x_size, y_size );

        FrameBufferProperties props( FrameBufferProperties::get_default() );
        props.set_back_buffers( 0 );
        props.set_rgb_color( false );
        props.set_depth_bits( depth_bits );
        props.set_stereo( _win->is_stereo() );

        if ( tex_group[2] != nullptr )
        {
                props.set_aux_rgba( 1 );
        }
        if ( tex_group[3] != nullptr )
        {
                props.set_aux_rgba( 2 );
        }

        PT( GraphicsOutput ) buffer = g_base->_window->get_graphics_window()
                ->get_engine()->make_output( _win->get_pipe(), name, -1, props, winprops,
                                             GraphicsPipe::BF_refuse_window | GraphicsPipe::BF_resizeable,
                                             _win->get_gsg(), _win );
        if ( buffer == nullptr )
        {
                return buffer;
        }

        if ( tex_group[0] != nullptr )
        {
                buffer->add_render_texture( tex_group[0], GraphicsOutput::RTM_bind_or_copy, GraphicsOutput::RTP_depth );
        }
        if ( tex_group[1] != nullptr )
        {
                buffer->add_render_texture( tex_group[1], GraphicsOutput::RTM_bind_or_copy, GraphicsOutput::RTP_color );
        }
        if ( tex_group[2] != nullptr )
        {
                buffer->add_render_texture( tex_group[2], GraphicsOutput::RTM_bind_or_copy, GraphicsOutput::RTP_aux_rgba_0 );
        }
        if ( tex_group[3] != nullptr )
        {
                buffer->add_render_texture( tex_group[3], GraphicsOutput::RTM_bind_or_copy, GraphicsOutput::RTP_aux_rgba_1 );
        }

        buffer->set_sort( _next_sort );
        buffer->disable_clears();

        _next_sort++;

        return buffer;
}

void FilterManager::window_event( const Event *e, void *data )
{
        ( (FilterManager *)data )->resize_buffers();
}

void FilterManager::resize_buffers()
{
        for ( size_t i = 0; i < _buffers.size(); i++ )
        {
                LVector3 size = _sizes[i];
                LVector2 ssize = get_scaled_size( size[0], size[1], size[2] );
                DCAST( GraphicsBuffer, _buffers[i] )->set_size( ssize[0], ssize[1] );
        }
}

void FilterManager::cleanup()
{
        for ( size_t i = 0; i < _buffers.size(); i++ )
        {
                _buffers[i]->clear_render_textures();
                _engine->remove_window( _buffers[i] );
        }

        _buffers.clear();
        _sizes.clear();

        set_clears( _win, _w_clears );
        set_clears( _region, _r_clears );

        _cam_state = _cam_init;
        DCAST( Camera, _cam.node() )->set_initial_state( _cam_init );
        _region->set_camera( _cam );
        _next_sort = _win->get_sort() - 1000;
        _base_x = 0;
        _base_y = 0;
}

NodePath &FilterManager::get_cam()
{
        return _cam;
}