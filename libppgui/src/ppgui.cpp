#include "ppGui.h"

#include "ppGuiScheme.h"
#include "ppGuiLabel.h"
#include "ppGuiImage.h"
#include "ppGuiButton.h"
#include "ppGuiScrollFrame.h"
#include "ppGuiEntry.h"
#include <vifParser.h>
#include <vifTokenizer.h>
#include <audioManager.h>
#include <texturePool.h>

#include <filename.h>

#include <fontPool.h>

#include "pp_globals.h"

#include <virtualFileSystem.h>

pvector<FontDef> Gui::_fonts;
pvector<SoundDef> Gui::_sounds;
pvector<TextureDef> Gui::_textures;

static TransparencyAttrib::Mode get_transparency( const string & );

// Apply the basic properties that all GuiItems should have.
static void apply_basic_props( Parser &parser, Object &obj, NodePath item )
{
        // Name
        if ( parser.has_property( obj, "name" ) )
        {
                item.set_name( parser.get_property_value( obj, "name" ) );
        }
        else
        {
                stringstream ss;
                ss << "GuiItem-" << item;
                item.set_name( ss.str() );
        }

        // Pos
        if ( parser.has_property( obj, "pos" ) )
        {
                vector<float> pos = parser.parse_float_list_str( parser.get_property_value( obj, "pos" ) );
                item.set_pos( pos[0], pos[1], pos[2] );
        }
        else
        {
                item.set_pos( 0, 0, 0 );
        }

        // Hpr
        if ( parser.has_property( obj, "hpr" ) )
        {
                vector<float> hpr = parser.parse_float_list_str( parser.get_property_value( obj, "hpr" ) );
                item.set_hpr( hpr[0], hpr[1], hpr[2] );
        }
        else
        {
                item.set_hpr( 0, 0, 0 );
        }

        // Scale
        if ( parser.has_property( obj, "scale" ) )
        {
                vector<float> scale = parser.parse_float_list_str( parser.get_property_value( obj, "scale" ) );
                item.set_scale( scale[0], scale[1], scale[2] );
        }
        else
        {
                item.set_scale( 1, 1, 1 );
        }

        // Transparency
        if ( parser.has_property( obj, "transparency" ) )
        {
                string transtr = parser.get_property_value( obj, "transparency" );
                TransparencyAttrib::Mode transmode = get_transparency( transtr );
                item.set_transparency( transmode, 1 );
        }

        // Color scale
        if ( parser.has_property( obj, "colorscale" ) )
        {
                vector<float> cs = parser.parse_float_list_str( parser.get_property_value( obj, "colorscale" ) );
                item.set_color_scale( cs[0], cs[1], cs[2], cs[3] );
        }
        else
        {
                item.set_color_scale( 1, 1, 1, 1 );
        }

        // Color
        if ( parser.has_property( obj, "color" ) )
        {
                vector<float> cl = parser.parse_float_list_str( parser.get_property_value( obj, "color" ) );
                item.set_color( cl[0], cl[1], cl[2], cl[3] );
        }
        else
        {
                item.set_color( 1, 1, 1, 1 );
        }

}

void Gui::
store_sound( const string &name, PT( AudioSound ) sound )
{
        SoundDef sd;
        sd.name = name;
        sd.sound = sound;
        _sounds.push_back( sd );
}

void Gui::
store_font( const string &name, PT( TextFont ) font )
{
        FontDef fd;
        fd.name = name;
        fd.font = font;
        _fonts.push_back( fd );
}

void Gui::
store_texture( const string &name, PT( Texture ) tex )
{
        TextureDef td;
        td.name = name;
        td.tex = tex;
        _textures.push_back( td );
}

PT( TextFont ) Gui::
get_font( const string &name )
{
        PT( TextFont ) result;
        for ( size_t i = 0; i < Gui::_fonts.size(); i++ )
        {
                FontDef font = Gui::_fonts[i];
                if ( font.name.compare( name ) == 0 )
                {
                        result = font.font;
                        break;
                }
        }
        return result;
}

PT( AudioSound ) Gui::
get_sound( const string &name )
{
        PT( AudioSound ) result;
        for ( size_t i = 0; i < Gui::_sounds.size(); i++ )
        {
                SoundDef sound = Gui::_sounds[i];
                if ( sound.name.compare( name ) == 0 )
                {
                        result = sound.sound;
                        break;
                }
        }
        return result;
}

PT( Texture ) Gui::
get_texture( const string &name )
{
        PT( Texture ) result;
        for ( size_t i = 0; i < Gui::_textures.size(); i++ )
        {
                TextureDef tex = Gui::_textures[i];
                if ( tex.name.compare( name ) == 0 )
                {
                        result = tex.tex;
                        break;
                }
        }
        return result;
}

static TextNode::Alignment get_align( const string &strformat )
{
        if ( strformat.compare( "left" ) == 0 )
        {
                return TextNode::A_left;
        }
        else if ( strformat.compare( "right" ) == 0 )
        {
                return TextNode::A_right;
        }
        else if ( strformat.compare( "center" ) == 0 )
        {
                return TextNode::A_center;
        }
        else if ( strformat.compare( "boxedleft" ) == 0 )
        {
                return TextNode::A_boxed_left;
        }
        else if ( strformat.compare( "boxedright" ) == 0 )
        {
                return TextNode::A_boxed_right;
        }
        else if ( strformat.compare( "boxedcenter" ) == 0 )
        {
                return TextNode::A_boxed_center;
        }
        cout << "'" << strformat << "' is not a valid text alignment type." << endl;
        return TextNode::A_center;
}

static TransparencyAttrib::Mode get_transparency( const string &transtr )
{
        if ( transtr.compare( "alpha" ) == 0 )
        {
                return TransparencyAttrib::M_alpha;
        }
        else if ( transtr.compare( "dual" ) == 0 || transtr.compare( "1" ) == 0 || transtr.compare( "true" ) == 0 )
        {
                // This is the default if you say 1 or true for the transparency.
                return TransparencyAttrib::M_dual;
        }
        else if ( transtr.compare( "multisample" ) == 0 )
        {
                return TransparencyAttrib::M_multisample;
        }
        else if ( transtr.compare( "none" ) == 0 )
        {
                return TransparencyAttrib::M_none;
        }
        else
        {
                cout << "'" << transtr << "' is not a valid transparency type." << endl;
                return TransparencyAttrib::M_none;
        }
}

static bool bool_from_str( const string &str )
{
        if ( str.compare( "1" ) == 0 )
        {
                return true;
        }
        return false;
}

// Walk all the schemes
static void walk_schemes( Parser &parser, vector<Object> &objs, GuiScheme *parent )
{
        for ( size_t i = 0; i < objs.size(); i++ )
        {
                Object obj = objs[i];
                PT( GuiScheme ) scheme = new GuiScheme( "" );
                parent->add_child( scheme );

                NodePath schemenode( scheme );

                schemenode.reparent_to( NodePath( parent ) );

                apply_basic_props( parser, obj, schemenode );

                // Find all of the GUI items part of this scheme (excluding other GuiSchemes).
                for ( size_t j = 0; j < obj.objects.size(); j++ )
                {
                        Object pobj = obj.objects[j];

                        if ( pobj.name.compare( "label" ) == 0 )
                        {
                                // This scheme contains a label.
                                PT( GuiLabel ) lbl = new GuiLabel( "" );

                                NodePath lblnode( lbl );

                                // Scale fix
                                if ( !parser.has_property( pobj, "scale" ) )
                                {
                                        lblnode.set_scale( .08, .08, .08 );
                                }

                                // Text
                                if ( parser.has_property( pobj, "text" ) )
                                {
                                        lbl->set_text( parser.get_property_value( pobj, "text" ) );
                                }
                                else
                                {
                                        lbl->set_text( "" );
                                }

                                // Text alignment
                                if ( parser.has_property( pobj, "align" ) )
                                {
                                        lbl->set_align( get_align( parser.get_property_value( pobj, "align" ) ) );
                                }
                                else
                                {
                                        lbl->set_align( TextNode::A_center );
                                }

                                // Shadow
                                if ( parser.has_property( pobj, "shadow" ) )
                                {
                                        vector<float> shadow = parser.parse_float_list_str( parser.get_property_value( pobj, "shadow" ) );
                                        lbl->set_shadow_color( shadow[0], shadow[1], shadow[2], shadow[3] );
                                }
                                else
                                {
                                        lbl->set_shadow_color( 0, 0, 0, 0 );
                                }

                                // Shadow offset
                                if ( parser.has_property( pobj, "shadowoffset" ) )
                                {
                                        vector<float> shoffset = parser.parse_float_list_str( parser.get_property_value( pobj, "shadowoffset" ) );
                                        lbl->set_shadow( shoffset[0], shoffset[1] );
                                }
                                else
                                {
                                        lbl->set_shadow( .04, .04 ); // <- make the correct default shadow offset
                                }

                                // Slant
                                if ( parser.has_property( pobj, "slant" ) )
                                {
                                        float slant = stof( parser.get_property_value( pobj, "slant" ) );
                                        lbl->set_slant( slant );
                                }
                                else
                                {
                                        lbl->set_slant( 0.0 );
                                }

                                // Text color
                                if ( parser.has_property( pobj, "textcolor" ) )
                                {
                                        vector<float> tcolor = parser.parse_float_list_str( parser.get_property_value( pobj, "textcolor" ) );
                                        lbl->set_text_color( tcolor[0], tcolor[1], tcolor[2], tcolor[3] );
                                }
                                else
                                {
                                        lbl->set_text_color( 1, 1, 1, 1 );
                                }

                                // Frame color
                                if ( parser.has_property( pobj, "framecolor" ) )
                                {
                                        vector<float> fcolor = parser.parse_float_list_str( parser.get_property_value( pobj, "framecolor" ) );
                                        lbl->set_frame_color( fcolor[0], fcolor[1], fcolor[2], fcolor[3] );
                                }
                                else
                                {
                                        lbl->set_frame_color( 0, 0, 0, 0 );
                                }

                                // Small caps
                                if ( parser.has_property( pobj, "smallcaps" ) )
                                {
                                        bool scaps = bool_from_str( parser.get_property_value( pobj, "smallcaps" ) );
                                        lbl->set_small_caps( scaps );
                                }
                                else
                                {
                                        lbl->set_small_caps( false );
                                }

                                // Small caps scale
                                if ( parser.has_property( pobj, "smallcaps_scale" ) )
                                {
                                        float scscale = stof( parser.get_property_value( pobj, "smallcaps_scale" ) );
                                        lbl->set_small_caps_scale( scscale );
                                }
                                else
                                {
                                        lbl->set_small_caps_scale( 1.0 );
                                }

                                // Font
                                if ( parser.has_property( pobj, "font" ) )
                                {
                                        string font = parser.get_property_value( pobj, "font" );
                                        // Hopefully the font they want was loaded.
                                        PT( TextFont ) ptfont = Gui::get_font( font );
                                        if ( ptfont != NULL )
                                        {
                                                lbl->set_font( ptfont );
                                        }
                                }

                                PT( PandaNode ) result = lbl->generate();
                                NodePath resultnode( result );
                                apply_basic_props( parser, pobj, resultnode );
                                resultnode.reparent_to( schemenode );
                        }

                        else if ( pobj.name.compare( "image" ) == 0 )
                        {
                                // This is an image.
                                PT( GuiImage ) img = new GuiImage( "" );

                                NodePath imgnode( img );

                                apply_basic_props( parser, pobj, imgnode );

                                // Image
                                if ( parser.has_property( pobj, "image" ) )
                                {
                                        string img_name = parser.get_property_value( pobj, "image" );
                                        Filename testfile( img_name );
                                        if ( testfile.exists() )
                                        {
                                                // They provided a file path.
                                                img->set_image( img_name );
                                        }
                                        else
                                        {
                                                // They provided the name of a texture that is currently stored.
                                                PT( Texture ) tex = Gui::get_texture( img_name );
                                                if ( tex != NULL )
                                                {
                                                        img->set_image( tex );
                                                }
                                                else
                                                {
                                                        cout << "Gui: no stored texture with name " << img_name << endl;
                                                }
                                        }
                                }
                                imgnode.reparent_to( schemenode );
                        }
                        else if ( pobj.name.compare( "button" ) == 0 )
                        {
                                // This is a good ole button.
                                PT( PPGuiButton ) btn = new PPGuiButton( "" );
                                NodePath btnnode( btn );
                                apply_basic_props( parser, pobj, btnnode );

                                if ( parser.has_property( pobj, "clicksound" ) )
                                {
                                        btn->set_click_sound( Gui::get_sound( parser.get_property_value( pobj, "clicksound" ) ) );
                                }

                                if ( parser.has_property( pobj, "hoversound" ) )
                                {
                                        btn->set_rollover_sound( Gui::get_sound( parser.get_property_value( pobj, "hoversound" ) ) );
                                }

                                if ( parser.has_property( pobj, "textfont" ) )
                                {
                                        btn->get_text()->set_font( Gui::get_font( parser.get_property_value( pobj, "textfont" ) ) );
                                }

                                if ( parser.has_property( pobj, "textscale" ) )
                                {
                                        PN_stdfloat scale = stof( parser.get_property_value( pobj, "textscale" ) );
                                        btn->get_text()->set_text_scale( scale );
                                }

                                if ( parser.has_property( pobj, "textpos" ) )
                                {
                                        vector<float> p = Parser::parse_float_list_str( parser.get_property_value( pobj, "textpos" ) );
                                        btn->set_text_pos( LPoint3f( p[0], p[1], p[2] ) );
                                }

                                if ( parser.has_property( pobj, "text0pos" ) )
                                {
                                        vector<float> p = Parser::parse_float_list_str( parser.get_property_value( pobj, "text0pos" ) );
                                        btn->set_text0_pos( LPoint3f( p[0], p[1], p[2] ) );
                                }

                                if ( parser.has_property( pobj, "text1pos" ) )
                                {
                                        vector<float> p = Parser::parse_float_list_str( parser.get_property_value( pobj, "text1pos" ) );
                                        btn->set_text1_pos( LPoint3f( p[0], p[1], p[2] ) );
                                }

                                if ( parser.has_property( pobj, "text2pos" ) )
                                {
                                        vector<float> p = Parser::parse_float_list_str( parser.get_property_value( pobj, "text2pos" ) );
                                        btn->set_text2_pos( LPoint3f( p[0], p[1], p[2] ) );
                                }

                                if ( parser.has_property( pobj, "text3pos" ) )
                                {
                                        vector<float> p = Parser::parse_float_list_str( parser.get_property_value( pobj, "text3pos" ) );
                                        btn->set_text3_pos( LPoint3f( p[0], p[1], p[2] ) );
                                }

                                if ( parser.has_property( pobj, "textcolor" ) )
                                {
                                        vector<float> tc = Parser::parse_float_list_str( parser.get_property_value( pobj, "textcolor" ) );
                                        btn->set_text_color( LColor( tc[0], tc[1], tc[2], tc[3] ) );
                                }

                                if ( parser.has_property( pobj, "text0color" ) )
                                {
                                        vector<float> t0 = Parser::parse_float_list_str( parser.get_property_value( pobj, "text0color" ) );
                                        btn->set_text0_color( LColorf( t0[0], t0[1], t0[2], t0[3] ) );
                                }

                                if ( parser.has_property( pobj, "text1color" ) )
                                {
                                        vector<float> t1 = Parser::parse_float_list_str( parser.get_property_value( pobj, "text1color" ) );
                                        btn->set_text1_color( LColorf( t1[0], t1[1], t1[2], t1[3] ) );
                                }

                                if ( parser.has_property( pobj, "text2color" ) )
                                {
                                        vector<float> t2 = Parser::parse_float_list_str( parser.get_property_value( pobj, "text2color" ) );
                                        btn->set_text2_color( LColorf( t2[0], t2[1], t2[2], t2[3] ) );
                                }

                                if ( parser.has_property( pobj, "text3color" ) )
                                {
                                        vector<float> t3 = Parser::parse_float_list_str( parser.get_property_value( pobj, "text3color" ) );
                                        btn->set_text3_color( LColorf( t3[0], t3[1], t3[2], t3[3] ) );
                                }

                                if ( parser.has_property( pobj, "textshadow" ) )
                                {
                                        vector<float> ts = Parser::parse_float_list_str( parser.get_property_value( pobj, "textshadow" ) );
                                        btn->get_text()->set_shadow( .04, .04 );
                                        btn->get_text()->set_shadow_color( LColor( ts[0], ts[1], ts[2], ts[3] ) );
                                }

                                if ( parser.has_property( pobj, "text" ) )
                                {
                                        btn->set_text( parser.get_property_value( pobj, "text" ) );
                                }

                                if ( parser.has_property( pobj, "idle" ) )
                                {
                                        btn->set_idle( Gui::get_texture( parser.get_property_value( pobj, "idle" ) ) );
                                }
                                if ( parser.has_property( pobj, "rlvr" ) )
                                {
                                        btn->set_rollover( Gui::get_texture( parser.get_property_value( pobj, "rlvr" ) ) );
                                }
                                if ( parser.has_property( pobj, "down" ) )
                                {
                                        btn->set_down( Gui::get_texture( parser.get_property_value( pobj, "down" ) ) );
                                }
                                if ( parser.has_property( pobj, "inactive" ) )
                                {
                                        btn->set_disabled( Gui::get_texture( parser.get_property_value( pobj, "inactive" ) ) );
                                }

                                if ( parser.has_property( pobj, "geomscale" ) )
                                {
                                        vector<float> gscale = Parser::parse_float_list_str( parser.get_property_value( pobj, "geomscale" ) );
                                        btn->set_geom_scale( LVector3f( gscale[0], gscale[1], gscale[2] ) );
                                }

                                if ( parser.has_property( pobj, "fitGeomAroundText" ) )
                                {
                                        string fit = parser.get_property_value( pobj, "fitGeomAroundText" );
                                        if ( fit.compare( "1" ) == 0 )
                                        {
                                                btn->set_fit_to_text( true );
                                        }
                                }
                                else
                                {
                                        btn->set_fit_to_text( true );
                                }

                                if ( parser.has_property( pobj, "geomOffset" ) )
                                {
                                        vector<float> offsetvec = Parser::parse_float_list_str( parser.get_property_value( pobj, "geomOffset" ) );
                                        btn->set_geom_offset( LVector4f( offsetvec[0], offsetvec[1], offsetvec[2], offsetvec[3] ) );
                                }

                                btn->set_text_only( true );
                                if ( parser.has_property( pobj, "textonly" ) )
                                {
                                        string nf = parser.get_property_value( pobj, "textonly" );
                                        if ( nf.compare( "0" ) == 0 )
                                        {
                                                btn->set_text_only( false );
                                        }
                                }

                                btn->setup();
                                btnnode.reparent_to( schemenode );
                        }
                        else if ( pobj.name.compare( "scrollFrame" ) == 0 )
                        {
                                // A scrolled frame.
                                PT( PPGuiScrollFrame ) pSFrame = new PPGuiScrollFrame( "" );
                                NodePath npFrame( pSFrame );
                                apply_basic_props( parser, pobj, npFrame );

                                float flScrollBarWidth = 0.08;
                                if ( parser.has_property( pobj, "scrollBarWidth" ) )
                                {
                                        flScrollBarWidth = stof( parser.get_property_value( pobj, "scrollBarWidth" ) );
                                }

                                float flBevel = 0.0;
                                if ( parser.has_property( pobj, "bevel" ) )
                                {
                                        flBevel = stof( parser.get_property_value( pobj, "bevel" ) );
                                }

                                vector<float> virtualFrame;
                                if ( parser.has_property( pobj, "canvasSize" ) )
                                {
                                        virtualFrame = parser.parse_float_list_str( parser.get_property_value( pobj, "canvasSize" ) );
                                }
                                else
                                {
                                        virtualFrame.push_back( -0.5 );
                                        virtualFrame.push_back( 0.5 );
                                        virtualFrame.push_back( -0.5 );
                                        virtualFrame.push_back( 0.5 );
                                }

                                vector<float> widthHeight;
                                if ( parser.has_property( pobj, "widthHeight" ) )
                                {
                                        widthHeight = parser.parse_float_list_str( parser.get_property_value( pobj, "widthHeight" ) );
                                }
                                else
                                {
                                        widthHeight.push_back( 1.0 );
                                        widthHeight.push_back( 1.0 );
                                }

                                pSFrame->setup( widthHeight[0], widthHeight[1], virtualFrame[0], virtualFrame[1],
                                                virtualFrame[2], virtualFrame[3], flScrollBarWidth, flBevel );

                                PGSliderBar *pVSlider = pSFrame->get_vertical_slider();
                                PGSliderBar *pHSlider = pSFrame->get_horizontal_slider();

                                PGFrameStyle style = pVSlider->get_thumb_button()->get_frame_style( 0 );
                                style.set_color( 0.9, 0.9, 0.9, 1.0 );

                                pVSlider->get_thumb_button()->set_frame_style( 0, style );
                                pVSlider->get_left_button()->set_frame_style( 0, style );
                                pVSlider->get_right_button()->set_frame_style( 0, style );

                                pHSlider->get_thumb_button()->set_frame_style( 0, style );
                                pHSlider->get_left_button()->set_frame_style( 0, style );
                                pHSlider->get_right_button()->set_frame_style( 0, style );

                                style = pVSlider->get_thumb_button()->get_frame_style( 1 );
                                style.set_color( 0.7, 0.7, 0.7, 1.0 );

                                pVSlider->get_thumb_button()->set_frame_style( 1, style );
                                pVSlider->get_left_button()->set_frame_style( 1, style );
                                pVSlider->get_right_button()->set_frame_style( 1, style );

                                pHSlider->get_thumb_button()->set_frame_style( 1, style );
                                pHSlider->get_left_button()->set_frame_style( 1, style );
                                pHSlider->get_right_button()->set_frame_style( 1, style );

                                style = pVSlider->get_thumb_button()->get_frame_style( 2 );
                                style.set_color( 1.0, 1.0, 1.0, 1.0 );

                                pVSlider->get_thumb_button()->set_frame_style( 2, style );
                                pVSlider->get_left_button()->set_frame_style( 2, style );
                                pVSlider->get_right_button()->set_frame_style( 2, style );

                                pHSlider->get_thumb_button()->set_frame_style( 2, style );
                                pHSlider->get_left_button()->set_frame_style( 2, style );
                                pHSlider->get_right_button()->set_frame_style( 2, style );


                                npFrame.reparent_to( schemenode );
                        }
                        else if ( pobj.name.compare( "entry" ) == 0 )
                        {
                                // It's a text entry box.
                                PT( PPGuiEntry ) pEntry = new PPGuiEntry( "" );

                                PT( TextNode ) pTextDef = new TextNode( "ppguientry_textdef" );
                                pTextDef->set_align( TextNode::A_left );
                                pTextDef->set_shadow( .04, .04 );

                                pEntry->set_text_def( 0, pTextDef );
                                pEntry->set_text_def( 1, pTextDef );
                                pEntry->set_text_def( 2, pTextDef );
                                pEntry->set_text_def( 3, pTextDef );

                                NodePath npEntry( pEntry );
                                apply_basic_props( parser, pobj, npEntry );

                                if ( parser.has_property( pobj, "numlines" ) )
                                {
                                        pEntry->set_num_lines( stoi( parser.get_property_value( pobj, "numlines" ) ) );
                                }
                                else
                                {
                                        pEntry->set_num_lines( 1 );
                                }

                                if ( parser.has_property( pobj, "overflowmode" ) )
                                {
                                        pEntry->set_overflow_mode( bool_from_str( parser.get_property_value( pobj, "overflow" ) ) );
                                }
                                else
                                {
                                        pEntry->set_overflow_mode( false );
                                }

                                if ( parser.has_property( pobj, "blinkrate" ) )
                                {
                                        pEntry->set_blink_rate( stof( parser.get_property_value( pobj, "blinkrate" ) ) );
                                }
                                else
                                {
                                        pEntry->set_blink_rate( 1.0 );
                                }

                                if ( parser.has_property( pobj, "maxchars" ) )
                                {
                                        pEntry->set_max_chars( stoi( parser.get_property_value( pobj, "maxchars" ) ) );
                                }
                                else
                                {
                                        pEntry->set_max_chars( 0 );
                                }

                                if ( parser.has_property( pobj, "maxwidth" ) )
                                {
                                        pEntry->set_max_width( stof( parser.get_property_value( pobj, "maxwidth" ) ) );
                                }
                                else
                                {
                                        pEntry->set_max_width( 0.0 );
                                }

                                if ( parser.has_property( pobj, "obscuremode" ) )
                                {
                                        pEntry->set_obscure_mode( bool_from_str( parser.get_property_value( pobj, "obscuremode" ) ) );
                                }
                                else
                                {
                                        pEntry->set_obscure_mode( false );
                                }

                                if ( parser.has_property( pobj, "cursorkeys" ) )
                                {
                                        pEntry->set_cursor_keys_active( bool_from_str( parser.get_property_value( pobj, "cursorkeys" ) ) );
                                }
                                else
                                {
                                        pEntry->set_cursor_keys_active( true );
                                }

                                if ( parser.has_property( pobj, "focus" ) )
                                {
                                        pEntry->set_focus( bool_from_str( parser.get_property_value( pobj, "focus" ) ) );
                                }
                                else
                                {
                                        pEntry->set_focus( false );
                                }

                                if ( parser.has_property( pobj, "backgroundfocus" ) )
                                {
                                        pEntry->set_background_focus( bool_from_str( parser.get_property_value( pobj, "backgroundfocus" ) ) );
                                }
                                else
                                {
                                        pEntry->set_background_focus( true );
                                }

                                if ( parser.has_property( pobj, "initialtext" ) )
                                {
                                        pEntry->set_text( parser.get_property_value( pobj, "initialtext" ) );
                                        pEntry->set_cursor_position( pEntry->get_num_characters() );
                                }
                                else
                                {
                                        pEntry->set_text( "" );
                                        pEntry->set_cursor_position( 0 );
                                }

                                if ( parser.has_property( pobj, "textfont" ) )
                                {
                                        pTextDef->set_font( Gui::get_font( parser.get_property_value( pobj, "textfont" ) ) );
                                }

                                if ( parser.has_property( pobj, "textcolor" ) )
                                {
                                        vector<float> col = Parser::parse_float_list_str( parser.get_property_value( pobj, "textcolor" ) );
                                        pTextDef->set_text_color( col[0], col[1], col[2], col[3] );
                                }
                                else
                                {
                                        pTextDef->set_text_color( 0, 0, 0, 1 );
                                }

                                if ( parser.has_property( pobj, "textshadow" ) )
                                {
                                        vector<float> sh = Parser::parse_float_list_str( parser.get_property_value( pobj, "textshadow" ) );
                                        pTextDef->set_shadow_color( sh[0], sh[1], sh[2], sh[3] );
                                }
                                else
                                {
                                        pTextDef->set_shadow_color( 0, 0, 0, 0 );
                                }

                                pEntry->setup( pEntry->get_max_width(), pEntry->get_num_lines() );

                                npEntry.reparent_to( schemenode );

                        }
                }
                walk_schemes( parser, parser.get_objects_with_name( obj, "scheme" ), scheme );
        }
}

NodePath Gui::
load_scheme( const string &filename )
{

        VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();
        istream *file = vfs->open_read_file( filename, true );
        stringstream ss;
        ss << file->rdbuf();
        string data = ss.str();

        // Parse the .vif data
        TokenVec tokens = tokenizer( data );
        Parser parser = Parser( tokens );

        // We should first load any fonts/sounds that may have been specified in the base.
        for ( size_t i = 0; i < parser._base_objects.size(); i++ )
        {
                Object obj = parser._base_objects[i];
                if ( obj.name.compare( "font" ) == 0 )
                {
                        // New font!
                        string path = parser.get_property_value( obj, "file" );
                        string name = parser.get_property_value( obj, "name" );

                        PT( TextFont ) font = FontPool::load_font( path );

                        if ( parser.has_property( obj, "spacewidth" ) )
                        {
                                float spw = stof( parser.get_property_value( obj, "spacewidth" ) );
                                font->set_space_advance( spw );
                        }

                        if ( parser.has_property( obj, "lineheight" ) )
                        {
                                float lih = stof( parser.get_property_value( obj, "lineheight" ) );
                                font->set_line_height( lih );
                        }

                        store_font( name, font );
                }
                else if ( obj.name.compare( "sound" ) == 0 )
                {
                        // New sound!
                        string path = parser.get_property_value( obj, "file" );
                        string name = parser.get_property_value( obj, "name" );

                        PT( AudioSound ) sound = g_sfx_mgr->get_sound( path );

                        if ( parser.has_property( obj, "volume" ) )
                        {
                                float vol = stof( parser.get_property_value( obj, "volume" ) );
                                sound->set_volume( vol );
                        }

                        store_sound( name, sound );
                }
                else if ( obj.name.compare( "texture" ) == 0 )
                {
                        string path = parser.get_property_value( obj, "file" );
                        string name = parser.get_property_value( obj, "name" );

                        PT( Texture ) tex = TexturePool::load_texture( path );

                        store_texture( name, tex );
                }
        }

        vector<Object> base_schemes = parser.get_base_objects_with_name( "scheme" );

        if ( base_schemes.size() == 0 )
        {
                // There are 0 schemes in this file, maybe they were just loading up some fonts or something?
                return NodePath();
        }

        PT( GuiScheme ) base_scheme = new GuiScheme( "baseScheme" );

        // Walk through every single Object and create the GUI schemes and elements.
        walk_schemes( parser, base_schemes, base_scheme );

        return NodePath( base_scheme );
}