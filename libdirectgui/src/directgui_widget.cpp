#include "directgui_widget.h"
#include "pp_globals.h"

DirectGuiWidget::DirectGuiWidget() :
        _num_states( 1 ),
        _sort_order( 0 ),
        _state( S_normal ),
        _relief( R_flat ),
        _border_width( 0.1, 0.1 ),
        _border_uv_width( 0.1, 0.1 ),
        _frame_size( 1, -1, 1, -1 ),
        _frame_color( 0.8, 0.8, 0.8, 1.0 ),
        _frame_visible_scale( 1, 1 ),
        _auto_frame_size( true ),
        _f_init( false ),
        _bounds( 0, 0, 0, 0 ),
        _ll( 0, 0, 0 ),
        _ur( 0, 0, 0 ),
        _pad( 0, 0 ),
        _suppress_mouse( true ),
        _suppress_keys( false ),
        _gui_item( new PGItem( "" ) )
{
}

string DirectGuiWidget::get_gui_id() const
{
        return _gui_item->get_id();
}

void DirectGuiWidget::set_state( int state )
{
        _state = state;
        _gui_item->set_active( (bool)state );
}

void DirectGuiWidget::reset_frame_size()
{
        if ( !_f_init )
        {
                set_frame_size( _frame_size, true );
        }
}

PGFrameStyle::Type DirectGuiWidget::get_frame_type( int state ) const
{
        return _frame_style[state].get_type();
}

PN_stdfloat DirectGuiWidget::get_width() const
{
        return _bounds[1] - _bounds[0];
}

PN_stdfloat DirectGuiWidget::get_height() const
{
        return _bounds[3] - _bounds[2];
}

LVector2f DirectGuiWidget::get_center() const
{
        return LVector2f( _bounds[0] + get_width() / 2.0,
                          _bounds[2] + get_height() / 2.0 );
}

LVector4f DirectGuiWidget::get_bounds( int state )
{
        _state_nodepath[state].calc_tight_bounds( _ll, _ur );
        LVector3f vec_right = LVector3f::right();
        LVector3f vec_up = LVector3f::up();
        PN_stdfloat left = ( vec_right[0] * _ll[0]
                             + vec_right[1] * _ll[1]
                             + vec_right[2] * _ll[2] );
        PN_stdfloat right = ( vec_right[0] * _ur[0]
                              + vec_right[1] * _ur[1]
                              + vec_right[2] * _ur[2] );
        PN_stdfloat bottom = ( vec_up[0] * _ll[0]
                               + vec_up[1] * _ll[1]
                               + vec_up[2] * _ll[2] );
        PN_stdfloat top = ( vec_up[0] * _ur[0]
                            + vec_up[1] * _ur[1]
                            + vec_up[2] * _ur[2] );
        _ll.set( left, 0.0, bottom );
        _ur.set( right, 0.0, top );
        _bounds.set( _ll[0] - _pad[0],
                     _ur[0] + _pad[0],
                     _ll[2] - _pad[1],
                     _ur[2] + _pad[1] );
        return _bounds;
}

void DirectGuiWidget::set_frame_size( const LVector4f &size, bool clear_frame )
{
        PGFrameStyle::Type type = get_frame_type();

        LVector2f bw( 0, 0 );

        if ( !_auto_frame_size )
        {
                // User specified bounds
                _bounds = _frame_size;
        }
        else
        {
                if ( clear_frame && ( type != PGFrameStyle::T_none ) )
                {
                        _frame_style[0].set_type( PGFrameStyle::T_none );
                        _gui_item->set_frame_style( 0, _frame_style[0] );
                        _gui_item->get_state_def( 0 );
                }
                get_bounds();
                if ( type != PGFrameStyle::T_none )
                {
                        _frame_style[0].set_type( type );
                        _gui_item->set_frame_style( 0, _frame_style[0] );
                }
                if ( type != PGFrameStyle::T_none &&
                     type != PGFrameStyle::T_flat )
                {
                        bw = _border_width;
                }
        }

        _gui_item->set_frame( _bounds[0] - bw[0],
                              _bounds[1] + bw[0],
                              _bounds[2] - bw[1],
                              _bounds[3] + bw[1] );
}

void DirectGuiWidget::initialize_options()
{
        set_state( _state );
        set_relief( _relief );
        set_border_width( _border_width );
        set_border_uv_width( _border_uv_width );
        set_frame_size( _frame_size );
        set_frame_color( _frame_color );
        set_frame_texture( _frame_texture );
        set_frame_visible_scale( _frame_visible_scale );

        update_frame_style();
        if ( !_auto_frame_size )
        {
                reset_frame_size();
        }
}

void DirectGuiWidget::update_frame_style()
{
        if ( !_f_init )
        {
                for ( int i = 0; i < _num_states; i++ )
                {
                        _gui_item->set_frame_style( i, _frame_style[i] );
                }
        }
}

void DirectGuiWidget::setup()
{
        NodePath::operator=( g_aspect2d.attach_new_node( _gui_item, _sort_order ) );

        _state_nodepath.clear();
        _frame_style.clear();
        for ( int i = 0; i < _num_states; i++ )
        {
                _state_nodepath.push_back( NodePath( _gui_item->get_state_def( i ) ) );
                _frame_style.push_back( PGFrameStyle() );
        }

        int suppress_flags = 0;
        if ( _suppress_mouse )
        {
                suppress_flags |= MouseWatcherRegion::SF_mouse_button;
                suppress_flags |= MouseWatcherRegion::SF_mouse_position;
        }
        if ( _suppress_keys )
        {
                suppress_flags |= MouseWatcherRegion::SF_other_button;
        }
        _gui_item->set_suppress_flags( suppress_flags );
}

void DirectGuiWidget::set_relief( int relief )
{
        _relief = relief;
        for ( int i = 0; i < _num_states; i++ )
        {
                _frame_style[i].set_type( ( PGFrameStyle::Type )relief );
        }
        update_frame_style();
}

void DirectGuiWidget::set_frame_color( const LColorf &color )
{
        _frame_color = color;
        for ( int i = 0; i < _num_states; i++ )
        {
                _frame_style[i].set_color( color );
        }
        update_frame_style();
}

void DirectGuiWidget::set_frame_texture( PT( Texture ) tex )
{
        pvector<PT( Texture )> texs;
        texs.push_back( tex );
        set_frame_texture( texs );
}

void DirectGuiWidget::set_frame_texture( const pvector<PT( Texture )> &texs )
{
        _frame_texture = texs;
        for ( int i = 0; i < _num_states; i++ )
        {
                Texture *tex = texs[0];
                if ( i < texs.size() )
                {
                        tex = texs[i];
                }

                if ( tex != nullptr )
                {
                        _frame_style[i].set_texture( tex );
                }
                else
                {
                        _frame_style[i].clear_texture();
                }

        }

        update_frame_style();
}

void DirectGuiWidget::set_frame_visible_scale( const LVector2f &scale )
{
        _frame_visible_scale = scale;
        for ( int i = 0; i < _num_states; i++ )
        {
                _frame_style[i].set_visible_scale( scale );
        }
        update_frame_style();
}

void DirectGuiWidget::set_border_width( const LVector2f &width )
{
        _border_width = width;
        for ( int i = 0; i < _num_states; i++ )
        {
                _frame_style[i].set_width( width );
        }
        update_frame_style();
}

void DirectGuiWidget::set_border_uv_width( const LVector2f &width )
{
        _border_uv_width = width;
        for ( int i = 0; i < _num_states; i++ )
        {
                _frame_style[i].set_uv_width( width );
        }
        update_frame_style();
}

void DirectGuiWidget::destroy()
{
        for ( size_t i = 0; i < _state_nodepath.size(); i++ )
        {
                _state_nodepath[i].remove_node();
        }
        _state_nodepath.clear();
        _frame_style.clear();
        remove_node();
}