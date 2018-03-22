#include "renderTarget.h"

#include "..\pp_globals.h"

#include <auxBitplaneAttrib.h>

int RenderTarget::num_allocated_buffers = 0;
bool RenderTarget::use_r11g11b10 = true;
pvector<PT( RenderTarget )> RenderTarget::registered_targets;
int RenderTarget::current_sort = -300;

RenderTarget::RenderTarget()
{
}

RenderTarget::RenderTarget( const string &name )
{
        _name = name;
        _color_bits.set( 0.0, 0.0, 0.0, 0.0 );
        _aux_bits = 8;
        _aux_count = 0;
        _depth_bits = 0;
        _size.set( -1, -1 );
        _size_constraint.set( -1, -1 );
        _active = false;

        _source_window = base->_window->get_graphics_window();

        // Disable all global clears, since they are not required
        int num_drs = _source_window->get_num_display_regions();
        for ( int i = 0; i < num_drs; i++ )
        {
                _source_window->get_display_region( i )->disable_clears();
        }
}

void RenderTarget::set_active( bool active )
{
        _active = active;
}

bool RenderTarget::get_active() const
{
        return _active;
}

PT( Texture ) RenderTarget::get_aux_tex( int n ) const
{
        return _aux_texs[n];
}

string RenderTarget::get_name() const
{
        return _name;
}

void RenderTarget::add_color_attachment( int bits, bool alpha )
{
        _color_target = new Texture( "_color" );
        _color_bits.set( bits, bits, bits, alpha ? bits : 0 );
}

void RenderTarget::add_color_attachment( LVecBase4i &bits, bool alpha )
{
        _color_target = new Texture( "_color" );
        _color_bits.set( bits[0], bits[1], bits[2], bits[3] );
}

void RenderTarget::add_depth_attachment( int bits )
{
        _depth_target = new Texture( "_depth" );
        _depth_bits = bits;
}

void RenderTarget::add_aux_attachment( int bits )
{
        _aux_bits = bits;
        _aux_count++;
}

void RenderTarget::add_aux_attachments( int bits, int count )
{
        _aux_bits = bits;
        _aux_count += count;
}

void RenderTarget::set_size( LVecBase2i &size )
{
        _size_constraint = size;
}

LVecBase2i RenderTarget::get_size() const
{
        return _size_constraint;
}

PT( Texture ) RenderTarget::get_color_tex() const
{
        return _color_target;
}

PT( Texture ) RenderTarget::get_depth_tex() const
{
        return _depth_target;
}

PT( GraphicsOutput ) RenderTarget::get_internal_buffer() const
{
        return _internal_buffer;
}

bool RenderTarget::create_buffer()
{
        compute_size_from_constraint();
        if ( !create() )
        {
                cout << "Failed to create buffer!" << endl;
                return false;
        }

        if ( _create_default_region )
        {
                _source_region_def = PostProcessRegion::make( _internal_buffer );

                int max = get_max_color_bits();
                if ( max == 0 )
                {
                        _source_region_def.set_attrib( ColorWriteAttrib::make( ColorWriteAttrib::M_none ), 1000 );
                }
        }

        return true;
}

void RenderTarget::set_clear_color( const LColor &color )
{
        _internal_buffer->set_clear_color_active( true );
        _internal_buffer->set_clear_color( color );
}

void RenderTarget::set_shader_input( const ShaderInput *inp )
{
        if ( _create_default_region )
        {
                _source_region_def.set_shader_input( inp );
        }
}

void RenderTarget::set_shader( const Shader *shad )
{
        if ( shad == NULL )
        {
                return;
        }

        _source_region_def.set_shader( shad );
}

int RenderTarget::get_max_color_bits() const
{
        int max = 0;

        if ( _color_bits.get_x() > _color_bits.get_y() &&
             _color_bits.get_x() > _color_bits.get_z() &&
             _color_bits.get_x() > _color_bits.get_w() )
        {
                max = _color_bits.get_x();
        }
        else if ( _color_bits.get_y() > _color_bits.get_x() &&
                  _color_bits.get_y() > _color_bits.get_z() &&
                  _color_bits.get_y() > _color_bits.get_w() )
        {
                max = _color_bits.get_y();
        }
        else if ( _color_bits.get_z() > _color_bits.get_z() &&
                  _color_bits.get_z() > _color_bits.get_y() &&
                  _color_bits.get_z() > _color_bits.get_w() )
        {
                max = _color_bits.get_z();
        }
        else if ( _color_bits.get_w() > _color_bits.get_x() &&
                  _color_bits.get_w() > _color_bits.get_z() &&
                  _color_bits.get_w() > _color_bits.get_y() )
        {
                max = _color_bits.get_w();
        }

        return max;
}

void RenderTarget::prepare_render( NodePath &camera )
{
        _create_default_region = false;
        create_buffer();
        _source_region = _internal_buffer->get_display_region( 0 );

        if ( !camera.is_empty() )
        {
                PT( Camera ) cam = DCAST( Camera, camera.node() );

                NodePath internal_state = NodePath( "rtis" );
                internal_state.set_state( cam->get_initial_state() );

                if ( _aux_count > 0 )
                {
                        internal_state.set_attrib( AuxBitplaneAttrib::make( _aux_bits ), 20 );
                }
                internal_state.set_attrib( TransparencyAttrib::make( TransparencyAttrib::M_none ), 20 );

                if ( get_max_color_bits() == 0 )
                {
                        internal_state.set_attrib( ColorWriteAttrib::make( ColorWriteAttrib::C_off ), 20 );
                }

                int num_drs = cam->get_num_display_regions();
                for ( int i = 0; i < num_drs; i++ )
                {
                        DCAST( DisplayRegion, cam->get_display_region( i ) )->set_active( true );
                }

                num_drs = _source_window->get_num_display_regions();
                for ( int i = 0; i < num_drs; i++ )
                {
                        PT( DisplayRegion ) region = _source_window->get_display_region( i );
                        if ( region->get_camera() == camera )
                        {
                                _source_window->remove_display_region( region );
                        }
                }

                cam->set_initial_state( internal_state.get_state() );
                _source_region->set_camera( camera );
        }

        _internal_buffer->disable_clears();
        _source_region->disable_clears();
        _source_region->set_active( true );
        _source_region->set_sort( 20 );

        _source_region->set_clear_depth_active( true );
        _source_region->set_clear_depth( 1.0 );
        _active = true;
}

void RenderTarget::prepare_buffer()
{
        create_buffer();
        _active = true;
}

void RenderTarget::present_on_screen()
{
        _source_region_def = PostProcessRegion::make( ( GraphicsOutput * )_source_window );
        _source_region_def.set_sort( 5 );
}

void RenderTarget::remove()
{
        _internal_buffer->clear_render_textures();
        _engine->remove_window( _internal_buffer );
        _active = false;
        if ( _color_target != NULL )
        {
                _color_target->release_all();
        }
        if ( _depth_target != NULL )
        {
                _depth_target->release_all();
        }
        for ( size_t i = 0; i < _aux_texs.size(); i++ )
        {
                _aux_texs[i]->release_all();
        }
        registered_targets.erase( find( registered_targets.begin(), registered_targets.end(), this ) );
}

void RenderTarget::compute_size_from_constraint()
{
        int x = base->get_resolution_x();
        int y = base->get_resolution_y();
        _size = _size_constraint;
        if ( _size_constraint.get_x() < 0 )
        {
                _size.set_x( ( x - _size_constraint.get_x() - 1 ) / -_size_constraint.get_x() );
        }
        else if ( _size_constraint.get_y() < 0 )
        {
                _size.set_y( ( x - _size_constraint.get_y() - 1 ) / -_size_constraint.get_y() );
        }
}

RenderTarget::PropertiesData RenderTarget::make_properties()
{
        WindowProperties winprop = WindowProperties::size( _size.get_x(), _size.get_y() );
        FrameBufferProperties bufprop;

        if ( _size_constraint.get_x() == 0 || _size_constraint.get_y() == 0 )
        {
                winprop = WindowProperties::size( 1, 1 );
        }

        if ( _color_bits.get_x() == 16 && _color_bits.get_y() == 16 &&
             _color_bits.get_z() == 16 && _color_bits.get_w() == 0 )
        {
                if ( RenderTarget::use_r11g11b10 )
                {
                        bufprop.set_rgba_bits( 11, 11, 10, 0 );
                }
                else
                {
                        bufprop.set_rgba_bits( _color_bits.get_x(), _color_bits.get_y(),
                                               _color_bits.get_z(), _color_bits.get_w() );
                }
        }
        else
        {
                bufprop.set_rgba_bits( _color_bits.get_x() == 8 ? 1 : _color_bits.get_x(),
                                       _color_bits.get_y() == 8 ? 1 : _color_bits.get_y(),
                                       _color_bits.get_z() == 8 ? 1 : _color_bits.get_z(),
                                       _color_bits.get_w() == 8 ? 1 : _color_bits.get_w() );
        }

        bufprop.set_accum_bits( 0 );
        bufprop.set_stencil_bits( 0 );
        bufprop.set_back_buffers( 0 );
        bufprop.set_coverage_samples( 0 );
        bufprop.set_depth_bits( _depth_bits );

        if ( _depth_bits == 32 )
        {
                bufprop.set_float_depth( true );
        }

        bufprop.set_float_color( ( _color_bits.get_x() > 8 || _color_bits.get_y() > 8 ||
                                   _color_bits.get_z() > 8 || _color_bits.get_w() > 8 ) );

        bufprop.set_force_hardware( true );
        bufprop.set_multisamples( 0 );
        bufprop.set_srgb_color( false );
        bufprop.set_stereo( false );
        bufprop.set_stencil_bits( 0 );

        switch ( _aux_bits )
        {
        case 8:
        case 16:
        case 32:
                bufprop.set_aux_rgba( _aux_count );
                break;
        }

        PropertiesData pdata;
        pdata.bufprop = bufprop;
        pdata.winprop = winprop;

        return pdata;
}

void RenderTarget::setup_textures()
{
        for ( int i = 0; i < _aux_count; i++ )
        {
                PT( Texture ) tex = new Texture( "aux" + to_string( ( long long )i ) );
                tex->set_wrap_u( SamplerState::WM_clamp );
                tex->set_wrap_v( SamplerState::WM_clamp );
                tex->set_anisotropic_degree( 0 );
                tex->set_x_size( _size.get_x() );
                tex->set_y_size( _size.get_y() );
                tex->set_magfilter( SamplerState::FT_linear );
                tex->set_minfilter( SamplerState::FT_linear );

                _aux_texs.push_back( tex );
        }
}

bool RenderTarget::create()
{
        setup_textures();
        PropertiesData pdata = make_properties();

        _internal_buffer = _engine->make_output( _source_window->get_pipe(), _name, 1, pdata.bufprop,
                                                 pdata.winprop, GraphicsPipe::BF_refuse_window | GraphicsPipe::BF_resizeable,
                                                 _source_window->get_gsg(), _source_window );

        if ( _internal_buffer == ( GraphicsOutput * )NULL )
        {
                cout << "Failed to create buffer!" << endl;
                return false;
        }

        if ( _depth_bits > 0 )
        {
                _internal_buffer->add_render_texture( _depth_target, GraphicsOutput::RTM_bind_or_copy,
                                                      GraphicsOutput::RTP_depth );
        }

        if ( _color_bits.get_x() > 0 || _color_bits.get_y() > 0 ||
             _color_bits.get_z() > 0 || _color_bits.get_w() > 0 )
        {
                _internal_buffer->add_render_texture( _color_target, GraphicsOutput::RTM_bind_or_copy,
                                                      GraphicsOutput::RTP_color );
        }

        for ( int i = 0; i < _aux_count; i++ )
        {
                GraphicsOutput::RenderTexturePlane pl;
                if ( _aux_bits == 8 )
                {
                        if ( i == 0 )
                        {
                                pl = GraphicsOutput::RTP_aux_rgba_0;
                        }
                        else if ( i == 1 )
                        {
                                pl = GraphicsOutput::RTP_aux_rgba_1;
                        }
                        else if ( i == 2 )
                        {
                                pl = GraphicsOutput::RTP_aux_rgba_2;
                        }
                        else if ( i == 3 )
                        {
                                pl = GraphicsOutput::RTP_aux_rgba_3;
                        }
                }
                else if ( _aux_bits == 16 )
                {
                        if ( i == 0 )
                        {
                                pl = GraphicsOutput::RTP_aux_hrgba_0;
                        }
                        else if ( i == 1 )
                        {
                                pl = GraphicsOutput::RTP_aux_hrgba_1;
                        }
                        else if ( i == 2 )
                        {
                                pl = GraphicsOutput::RTP_aux_hrgba_2;
                        }
                        else if ( i == 3 )
                        {
                                pl = GraphicsOutput::RTP_aux_hrgba_3;
                        }
                }
                else if ( _aux_bits == 32 )
                {
                        if ( i == 0 )
                        {
                                pl = GraphicsOutput::RTP_aux_float_0;
                        }
                        else if ( i == 1 )
                        {
                                pl = GraphicsOutput::RTP_aux_float_1;
                        }
                        else if ( i == 2 )
                        {
                                pl = GraphicsOutput::RTP_aux_float_2;
                        }
                        else if ( i == 3 )
                        {
                                pl = GraphicsOutput::RTP_aux_float_3;
                        }
                }
                _internal_buffer->add_render_texture(
                        _aux_texs[i], GraphicsOutput::RTM_bind_or_copy, pl );
        }

        if ( _sort < 1 )
        {
                RenderTarget::current_sort += 20;
                _sort = RenderTarget::current_sort;
        }

        RenderTarget::num_allocated_buffers++;
        _internal_buffer->set_sort( _sort );
        _internal_buffer->disable_clears();
        _internal_buffer->get_display_region( 0 )->disable_clears();
        _internal_buffer->get_overlay_display_region()->disable_clears();
        _internal_buffer->get_overlay_display_region()->set_active( false );

        RenderTarget::registered_targets.push_back( this );
        return true;
}

void RenderTarget::consider_resize()
{
        LVecBase2i curr_size = _size;
        compute_size_from_constraint();
        if ( curr_size != _size )
        {
                if ( _internal_buffer != NULL )
                {
                        _internal_buffer->set_size_and_recalc( _size.get_x(), _size.get_y() );
                }
        }
}