#include "ppGuiButton.h"

#include <cardMaker.h>

#include <pp_globals.h>

#include <mouseButton.h>

#include <colorAttrib.h>

TypeDef( PPGuiButton );

PPGuiButton::
PPGuiButton( const string &name ) :
        PGButton( name ),
        _idle_tex( NULL ),
        _rlvr_tex( NULL ),
        _down_tex( NULL ),
        _disabled_tex( NULL ),
        _sound_click( NULL ),
        _sound_rlvr( NULL ),
        _geom_scale( LVector3f( 1 ) ),
        _geom_offset( LVector4f( 0 ) ),
        _fit_to_text( false ),
        _idle_np( "idle" ),
        _down_np( "down" ),
        _rlvr_np( "rlvr" ),
        _inac_np( "inactive" ),
        _setup( false ),
        _text0_color( 1.0, 0, 0, 1 ),
        _text1_color( 0, 0, 0, 1 ),
        _text2_color( 0, 0, 0, 1 ),
        _text3_color( 0, 0, 0, 1 ),
        _text0_pos( 0, 0, 0 ),
        _text1_pos( 0, 0, 0 ),
        _text2_pos( 0, 0, 0 ),
        _text3_pos( 0, 0, 0 ),
        _text_only( true )
{
        _text = new TextNode( "BTNTEXT" );
}

void PPGuiButton::
set_text_pos( const LPoint3f &pos )
{
        set_text0_pos( pos );
        set_text1_pos( pos );
        set_text2_pos( pos );
        set_text3_pos( pos );
}

void PPGuiButton::
set_text0_pos( const LPoint3f &pos )
{
        _text0_pos = pos;
}

void PPGuiButton::
set_text1_pos( const LPoint3f &pos )
{
        _text1_pos = pos;
}

void PPGuiButton::
set_text2_pos( const LPoint3f &pos )
{
        _text2_pos = pos;
}

void PPGuiButton::
set_text3_pos( const LPoint3f &pos )
{
        _text3_pos = pos;
}


void PPGuiButton::
set_text_color( LColorf &color )
{
        _text0_color = color;
        _text1_color = color;
        _text2_color = color;
        _text3_color = color;
}

void PPGuiButton::
set_text0_color( LColorf &color )
{
        _text0_color = color;
}

void PPGuiButton::
set_text1_color( LColorf &color )
{
        _text1_color = color;
}

void PPGuiButton::
set_text2_color( LColorf &color )
{
        _text2_color = color;
}

void PPGuiButton::
set_text3_color( LColorf &color )
{
        _text3_color = color;
}

void PPGuiButton::
set_text( const string &text )
{
        _text->set_text( text );
        if ( _setup )
        {
                // Wants to change the text after being setup.
                setup_text();
        }
}

TextNode *PPGuiButton::
get_text()
{
        return _text;
}

void PPGuiButton::
set_text_only( bool flag )
{
        _text_only = flag;
}

bool PPGuiButton::
get_text_only() const
{
        return _text_only;
}

void PPGuiButton::
press( const MouseWatcherParameter &param, bool background )
{
        PGButton::press( param, background );
        if ( _sound_click != NULL )
        {
                _sound_click->play();
        }

}

void PPGuiButton::
set_click_sound( PT( AudioSound ) s )
{
        _sound_click = s;
}

void PPGuiButton::
set_rollover_sound( PT( AudioSound ) s )
{
        _sound_rlvr = s;
        set_sound( get_enter_event(), _sound_rlvr );
}

NodePath PPGuiButton::
make_card( PT( Texture ) tex )
{
        CardMaker card( "texcard" );
        LVector4f frame( -0.5f, 0.5f, -0.5f, 0.5f );
        if ( _fit_to_text )
        {
                frame = _text->get_frame_actual();
        }
        card.set_frame( frame + _geom_offset );
        NodePath node( card.generate() );
        node.set_texture( tex, 1 );
        node.set_scale( _geom_scale );
        node.set_transparency( TransparencyAttrib::M_dual, 1 );
        return node;
}

void PPGuiButton::
setup_text()
{
        if ( _setup )
        {
                remove_text_from_state_nodes();
        }
        _text->set_align( TextNode::A_center );
        if ( _setup )
        {
                attach_text_to_state_nodes();
        }
}

void PPGuiButton::
remove_text_from_state_nodes()
{
        if ( _text != NULL )
        {
                _rlvr_np.find( "**/rlvrtextnode" ).remove_node();
                _idle_np.find( "**/idletextnode" ).remove_node();
                _down_np.find( "**/downtextnode" ).remove_node();
                _inac_np.find( "**/inactextnode" ).remove_node();
        }
}

void PPGuiButton::
attach_text_to_state_nodes()
{
        if ( _text != NULL )
        {
                PT( TextNode ) rlvr_text = new TextNode( "rlvrtextnode", *_text );
                rlvr_text->set_text( _text->get_text() );
                rlvr_text->set_text_color( _text2_color );

                PT( TextNode ) idle_text = new TextNode( "idletextnode", *_text );
                idle_text->set_text( _text->get_text() );
                idle_text->set_text_color( _text0_color );

                PT( TextNode ) down_text = new TextNode( "downtextnode", *_text );
                down_text->set_text( _text->get_text() );
                down_text->set_text_color( _text1_color );

                PT( TextNode ) inac_text = new TextNode( "inactextnode", *_text );
                inac_text->set_text( _text->get_text() );
                inac_text->set_text_color( _text3_color );

                _rlvr_np.attach_new_node( rlvr_text->generate() ).set_pos( _text2_pos );
                _idle_np.attach_new_node( idle_text->generate() ).set_pos( _text0_pos );
                _down_np.attach_new_node( down_text->generate() ).set_pos( _text1_pos );
                _inac_np.attach_new_node( inac_text->generate() ).set_pos( _text3_pos );
        }
}

void PPGuiButton::
setup()
{
        if ( _setup )
        {
                return;
        }

        clear_state_def( S_ready );
        clear_state_def( S_depressed );
        clear_state_def( S_rollover );
        clear_state_def( S_inactive );

        setup_text();

        bool has_rlvr_node = false;
        bool has_idle_node = false;
        bool has_down_node = false;
        bool has_inac_node = false;

        if ( _rlvr_tex != NULL )
        {
                has_rlvr_node = true;
                make_card( _rlvr_tex ).reparent_to( _rlvr_np );
        }
        if ( _idle_tex != NULL )
        {
                has_idle_node = true;
                make_card( _idle_tex ).reparent_to( _idle_np );
        }
        if ( _down_tex != NULL )
        {
                has_down_node = true;
                make_card( _down_tex ).reparent_to( _down_np );
        }
        if ( _disabled_tex != NULL )
        {
                has_inac_node = true;
                make_card( _disabled_tex ).reparent_to( _inac_np );
        }

        attach_text_to_state_nodes();

        if ( !has_idle_node && !has_rlvr_node && !has_down_node && !has_inac_node )
        {
                LVecBase4 frame = _text->get_card_actual();
                set_frame( frame[0] - 0.35, frame[1] + 0.35, frame[2] - 0.1f, frame[3] + 0.1f );

                if ( !_text_only )
                {

                        PT( PandaNode ) ready = new PandaNode( "ready" );
                        PT( PandaNode ) depressed = new PandaNode( "depressed" );
                        PT( PandaNode ) rollover = new PandaNode( "rollover" );
                        PT( PandaNode ) inactive = new PandaNode( "inactive" );

                        PGFrameStyle style;
                        style.set_color( 0.8f, 0.8f, 0.8f, 1.0f );
                        style.set_width( 0.08, 0.08 );

                        style.set_type( PGFrameStyle::T_bevel_out );
                        set_frame_style( S_ready, style );

                        style.set_color( 0.9f, 0.9f, 0.9f, 1.0f );
                        set_frame_style( S_rollover, style );

                        _inac_np.set_attrib( ColorAttrib::make_flat( LColor( 0.8f, 0.8f, 0.8f, 1.0f ) ) );
                        style.set_color( 0.6f, 0.6f, 0.6f, 1.0f );
                        set_frame_style( S_inactive, style );

                        style.set_type( PGFrameStyle::T_bevel_in );
                        style.set_color( 0.8f, 0.8f, 0.8f, 1.0f );
                        set_frame_style( S_depressed, style );

                        _idle_np.attach_new_node( ready, 1 );
                        _down_np.attach_new_node( depressed, 1 );
                        _rlvr_np.attach_new_node( rollover, 1 );
                        _inac_np.attach_new_node( inactive, 1 );
                }


                instance_to_state_def( S_ready, _idle_np );
                instance_to_state_def( S_depressed, _down_np );
                instance_to_state_def( S_rollover, _rlvr_np );
                instance_to_state_def( S_inactive, _inac_np );
        }
        else if ( has_idle_node && !has_rlvr_node && !has_down_node && !has_inac_node )
        {
                instance_to_state_def( S_ready, _idle_np );
                instance_to_state_def( S_depressed, _idle_np );
                instance_to_state_def( S_rollover, _idle_np );
                instance_to_state_def( S_inactive, _idle_np );

                LPoint3 min_point, max_point;
                _idle_np.calc_tight_bounds( min_point, max_point );
                set_frame( min_point[0], max_point[0],
                           min_point[2], max_point[2] );
        }
        else if ( has_idle_node && has_down_node && !has_rlvr_node && !has_inac_node )
        {
                instance_to_state_def( S_ready, _idle_np );
                instance_to_state_def( S_depressed, _down_np );
                instance_to_state_def( S_rollover, _idle_np );
                instance_to_state_def( S_inactive, _idle_np );

                LPoint3 min_point, max_point;
                _idle_np.calc_tight_bounds( min_point, max_point );
                set_frame( min_point[0], max_point[0],
                           min_point[2], max_point[2] );
        }
        else if ( has_idle_node && has_down_node && has_rlvr_node && !has_inac_node )
        {
                instance_to_state_def( S_ready, _idle_np );
                instance_to_state_def( S_depressed, _down_np );
                instance_to_state_def( S_rollover, _rlvr_np );
                instance_to_state_def( S_inactive, _idle_np );

                LPoint3 min_point, max_point;
                _idle_np.calc_tight_bounds( min_point, max_point );
                set_frame( min_point[0], max_point[0],
                           min_point[2], max_point[2] );
        }
        else if ( has_idle_node && has_down_node && has_rlvr_node && has_inac_node )
        {
                instance_to_state_def( S_ready, _idle_np );
                instance_to_state_def( S_depressed, _down_np );
                instance_to_state_def( S_rollover, _rlvr_np );
                instance_to_state_def( S_inactive, _inac_np );

                LPoint3 min_point, max_point;
                _idle_np.calc_tight_bounds( min_point, max_point );
                set_frame( min_point[0], max_point[0],
                           min_point[2], max_point[2] );

        }

        _setup = true;
}

void PPGuiButton::
set_fit_to_text( bool flag )
{
        _fit_to_text = flag;
}

void PPGuiButton::
set_geom_offset( LVector4f &offset )
{
        _geom_offset = offset;
}

void PPGuiButton::
set_geom_scale( LVector3f &scale )
{
        _geom_scale = scale;
}

void PPGuiButton::
set_idle( PT( Texture ) tex )
{
        _idle_tex = tex;
}

void PPGuiButton::
set_rollover( PT( Texture ) tex )
{
        _rlvr_tex = tex;
}

void PPGuiButton::
set_down( PT( Texture ) tex )
{
        _down_tex = tex;
}

void PPGuiButton::
set_disabled( PT( Texture ) tex )
{
        _disabled_tex = tex;
}