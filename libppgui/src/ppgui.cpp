#include "ppgui.h"
#include "ppguischeme.h"
#include "ppguilabel.h"
#include "ppguiimage.h"
#include "ppguibutton.h"
#include "ppguiscrollframe.h"
#include "ppguientry.h"
#include "pp_globals.h"
#include "pp_utils.h"
#include "vifparser.h"
#include "viftokenizer.h"

#include <audioManager.h>
#include <texturePool.h>
#include <filename.h>
#include <fontPool.h>
#include <virtualFileSystem.h>

pvector<FontDef> Gui::_fonts;
pvector<SoundDef> Gui::_sounds;
pvector<TextureDef> Gui::_textures;

static TransparencyAttrib::Mode get_transparency( const string & );

// Apply the basic properties that all GuiItems should have.
static void apply_basic_props( Parser &parser, Object &obj, NodePath item )
{
        for ( size_t i = 0; i < obj.properties.size(); i++ )
        {
                Property prop = obj.properties[i];
                if ( prop.name == "name" )
                {
                        item.set_name( prop.value );
                }
                else if ( prop.name == "pos" )
                {
                        vector<PN_stdfloat> pos = parser.parse_float_list_str( prop.value );
                        item.set_pos( pos[0], pos[1], pos[2] );
                }
                else if ( prop.name == "hpr" )
                {
                        vector<PN_stdfloat> hpr = parser.parse_float_list_str( prop.value );
                        item.set_hpr( hpr[0], hpr[1], hpr[2] );
                }
                else if ( prop.name == "scale" )
                {
                        vector<PN_stdfloat> scale = parser.parse_float_list_str( prop.value );
                        item.set_scale( scale[0], scale[1], scale[2] );
                }
                else if ( prop.name == "transparency" )
                {
                        TransparencyAttrib::Mode transmode = get_transparency( prop.value );
                        item.set_transparency( transmode, 1 );
                }
                else if ( prop.name == "colorscale" )
                {
                        vector<PN_stdfloat> cs = parser.parse_float_list_str( prop.value );
                        item.set_color_scale( cs[0], cs[1], cs[2], cs[3] );
                }
                else if ( prop.name == "color" )
                {
                        vector<PN_stdfloat> cl = parser.parse_float_list_str( prop.value );
                        item.set_color( cl[0], cl[1], cl[2], cl[3] );
                }
        }
}

void Gui::store_sound( const string &name, PT( AudioSound ) sound )
{
        SoundDef sd;
        sd.name = name;
        sd.sound = sound;
        _sounds.push_back( sd );
}

void Gui::store_font( const string &name, PT( DynamicTextFont ) font )
{
        FontDef fd;
        fd.name = name;
        fd.font = font;
        _fonts.push_back( fd );
}

void Gui::store_texture( const string &name, PT( Texture ) tex )
{
        TextureDef td;
        td.name = name;
        td.tex = tex;
        _textures.push_back( td );
}

PT( DynamicTextFont ) Gui::get_font( const string &name )
{
        PT( DynamicTextFont ) result;
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

PT( AudioSound ) Gui::get_sound( const string &name )
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

PT( Texture ) Gui::get_texture( const string &name )
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

static void make_label( Object &pobj, Parser &parser, const NodePath &schemenode )
{
        PT( GuiLabel ) lbl = new GuiLabel( "" );
        lbl->set_align( TextNode::A_center );
        lbl->set_shadow( 0.04, 0.04 );
        lbl->set_shadow_color( LColor( 0, 0, 0, 0 ) );

        NodePath lblnode( lbl );

        for ( size_t i = 0; i < pobj.properties.size(); i++ )
        {
                Property prop = pobj.properties[i];

                if ( prop.name == "text" )
                        lbl->set_text( prop.value );
                else if ( prop.name == "align" )
                        lbl->set_align( get_align( prop.value ) );
                else if ( prop.name == "shadow" )
                {
                        vector<PN_stdfloat> shadow = parser.parse_float_list_str( prop.value );
                        lbl->set_shadow_color( shadow[0], shadow[1], shadow[2], shadow[3] );
                }
                else if ( prop.name == "shadowoffset" )
                {
                        vector<PN_stdfloat> shoffset = parser.parse_float_list_str( prop.value );
                        lbl->set_shadow( shoffset[0], shoffset[1] );
                }
                else if ( prop.name == "slant" )
                {
                        lbl->set_slant( stof( prop.value ) );
                }
                else if ( prop.name == "textcolor" )
                {
                        vector<PN_stdfloat> tcolor = parser.parse_float_list_str( prop.value );
                        lbl->set_text_color( tcolor[0], tcolor[1], tcolor[2], tcolor[3] );
                }
                else if ( prop.name == "framecolor" )
                {
                        vector<PN_stdfloat> fcolor = parser.parse_float_list_str( prop.value );
                        lbl->set_frame_color( fcolor[0], fcolor[1], fcolor[2], fcolor[3] );
                }
                else if ( prop.name == "smallcaps" )
                {
                        bool scaps = bool_from_str( prop.value );
                        lbl->set_small_caps( scaps );
                }
                else if ( prop.name == "smallcaps_scale" )
                {
                        lbl->set_small_caps_scale( stof( prop.value ) );
                }
                else if ( prop.name == "font" )
                {
                        string font = prop.value;
                        // Hopefully the font they want was loaded.
                        PT( DynamicTextFont ) ptfont = Gui::get_font( font );
                        if ( ptfont != nullptr )
                        {
                                lbl->set_font( ptfont );
                        }
                }
        }

        PT( PandaNode ) result = lbl->generate();
        NodePath resultnode( result );
        apply_basic_props( parser, pobj, resultnode );
        resultnode.reparent_to( schemenode );
}

static void make_image( Object &pobj, Parser &parser, const NodePath &schemenode )
{
        PT( GuiImage ) img = new GuiImage( "" );

        NodePath imgnode( img );

        apply_basic_props( parser, pobj, imgnode );

        for ( size_t i = 0; i < pobj.properties.size(); i++ )
        {
                Property prop = pobj.properties[i];
                if ( prop.name == "image" )
                {
                        string img_name = prop.value;
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
                                if ( tex != nullptr )
                                {
                                        img->set_image( tex );
                                }
                                else
                                {
                                        cout << "Gui: no stored texture with name " << img_name << endl;
                                }
                        }
                }
        }

        imgnode.reparent_to( schemenode );
}

static void make_button( Object &pobj, Parser &parser, const NodePath &schemenode )
{
        PT( PPGuiButton ) btn = new PPGuiButton( "" );
        NodePath btnnode( btn );
        apply_basic_props( parser, pobj, btnnode );

        for ( size_t i = 0; i < pobj.properties.size(); i++ )
        {
                Property prop = pobj.properties[i];

                if ( prop.name == "clicksound" )
                {
                        btn->set_click_sound( Gui::get_sound( prop.value ) );
                }

                else if ( prop.name == "hoversound" )
                {
                        btn->set_rollover_sound( Gui::get_sound( prop.value ) );
                }

                else if ( prop.name == "clickevent" )
                {
                        btn->set_click_event( prop.value );
                }

                else if ( prop.name == "textfont" )
                {
                        btn->get_text()->set_font( Gui::get_font( prop.value ) );
                }

                else if ( prop.name == "textscale" )
                {
                        PN_stdfloat scale = stof( prop.value );
                        btn->get_text()->set_text_scale( scale );
                }

                else if ( prop.name == "textpos" )
                {
                        vector<PN_stdfloat> p = Parser::parse_float_list_str( prop.value );
                        btn->set_text_pos( LPoint3f( p[0], p[1], p[2] ) );
                }

                else if ( prop.name == "text0pos" )
                {
                        vector<PN_stdfloat> p = Parser::parse_float_list_str( prop.value );
                        btn->set_text0_pos( LPoint3f( p[0], p[1], p[2] ) );
                }

                else if ( prop.name == "text1pos" )
                {
                        vector<PN_stdfloat> p = Parser::parse_float_list_str( prop.value );
                        btn->set_text1_pos( LPoint3f( p[0], p[1], p[2] ) );
                }

                else if ( prop.name == "text2pos" )
                {
                        vector<PN_stdfloat> p = Parser::parse_float_list_str( prop.value );
                        btn->set_text2_pos( LPoint3f( p[0], p[1], p[2] ) );
                }

                else if ( prop.name == "text3pos" )
                {
                        vector<PN_stdfloat> p = Parser::parse_float_list_str( prop.value );
                        btn->set_text3_pos( LPoint3f( p[0], p[1], p[2] ) );
                }

                else if ( prop.name == "textcolor" )
                {
                        vector<PN_stdfloat> tc = Parser::parse_float_list_str( prop.value );
                        btn->set_text_color( LColor( tc[0], tc[1], tc[2], tc[3] ) );
                }

                else if ( prop.name == "text0color" )
                {
                        vector<PN_stdfloat> t0 = Parser::parse_float_list_str( prop.value );
                        btn->set_text0_color( LColorf( t0[0], t0[1], t0[2], t0[3] ) );
                }

                else if ( prop.name == "text1color" )
                {
                        vector<PN_stdfloat> t1 = Parser::parse_float_list_str( prop.value );
                        btn->set_text1_color( LColorf( t1[0], t1[1], t1[2], t1[3] ) );
                }

                else if ( prop.name == "text2color" )
                {
                        vector<PN_stdfloat> t2 = Parser::parse_float_list_str( prop.value );
                        btn->set_text2_color( LColorf( t2[0], t2[1], t2[2], t2[3] ) );
                }

                else if ( prop.name == "text3color" )
                {
                        vector<PN_stdfloat> t3 = Parser::parse_float_list_str( prop.value );
                        btn->set_text3_color( LColorf( t3[0], t3[1], t3[2], t3[3] ) );
                }

                else if ( prop.name == "textshadow" )
                {
                        vector<PN_stdfloat> ts = Parser::parse_float_list_str( prop.value );
                        btn->get_text()->set_shadow( .04, .04 );
                        btn->get_text()->set_shadow_color( LColor( ts[0], ts[1], ts[2], ts[3] ) );
                }

                else if ( prop.name == "text" )
                {
                        btn->set_text( prop.value );
                }

                else if ( prop.name == "idle" )
                {
                        btn->set_idle( Gui::get_texture( prop.value ) );
                }
                else if ( prop.name == "rlvr" )
                {
                        btn->set_rollover( Gui::get_texture( prop.value ) );
                }
                else if ( prop.name == "down" )
                {
                        btn->set_down( Gui::get_texture( prop.value ) );
                }
                else if ( prop.name == "inactive" )
                {
                        btn->set_disabled( Gui::get_texture( prop.value ) );
                }

                else if ( prop.name == "geomscale" )
                {
                        vector<PN_stdfloat> gscale = Parser::parse_float_list_str( prop.value );
                        btn->set_geom_scale( LVector3f( gscale[0], gscale[1], gscale[2] ) );
                }

                else if ( prop.name == "fitGeomAroundText" )
                {
                        string fit = prop.value;
                        if ( fit.compare( "1" ) == 0 )
                        {
                                btn->set_fit_to_text( true );
                        }
                }

                else if ( prop.name == "geomOffset" )
                {
                        vector<PN_stdfloat> offsetvec = Parser::parse_float_list_str( prop.value );
                        btn->set_geom_offset( LVector4f( offsetvec[0], offsetvec[1], offsetvec[2], offsetvec[3] ) );
                }

                else if ( prop.name == "textonly" )
                {
                        string nf = prop.value;
                        if ( nf.compare( "0" ) == 0 )
                        {
                                btn->set_text_only( false );
                        }
                }
        }

        btn->setup();
        btnnode.reparent_to( schemenode );
}

static void make_scrolled_frame( Object &pobj, Parser &parser, const NodePath &schemenode )
{
        PT( PPGuiScrollFrame ) sframe = new PPGuiScrollFrame( "" );
        NodePath frame_np( sframe );
        apply_basic_props( parser, pobj, frame_np );

        PN_stdfloat scrollbar_width = 0.08;
        PN_stdfloat bevel = 0.0;
        vector<PN_stdfloat> virtual_frame;
        virtual_frame.push_back( -0.5 );
        virtual_frame.push_back( 0.5 );
        virtual_frame.push_back( -0.5 );
        virtual_frame.push_back( 0.5 );
        vector<PN_stdfloat> width_height;
        width_height.push_back( 1.0 );
        width_height.push_back( 1.0 );

        for ( size_t i = 0; i < pobj.properties.size(); i++ )
        {
                Property prop = pobj.properties[i];

                if ( prop.name == "scrollbarwidth" )
                {
                        scrollbar_width = stof( prop.value );
                }
                else if ( prop.name == "bevel" )
                {
                        bevel = stof( prop.value );
                }
                else if ( prop.name == "canvassize" )
                {
                        virtual_frame = parser.parse_float_list_str( prop.value );
                }
                else if ( prop.name == "widthheight" )
                {
                        width_height = parser.parse_float_list_str( prop.value );
                }
        }

        sframe->setup( width_height[0], width_height[1], virtual_frame[0], virtual_frame[1],
                       virtual_frame[2], virtual_frame[3], scrollbar_width, bevel );

        PGSliderBar *v_slider = sframe->get_vertical_slider();
        PGSliderBar *h_slider = sframe->get_horizontal_slider();

        PGFrameStyle style = v_slider->get_thumb_button()->get_frame_style( 0 );
        style.set_color( 0.9, 0.9, 0.9, 1.0 );

        v_slider->get_thumb_button()->set_frame_style( 0, style );
        v_slider->get_left_button()->set_frame_style( 0, style );
        v_slider->get_right_button()->set_frame_style( 0, style );

        h_slider->get_thumb_button()->set_frame_style( 0, style );
        h_slider->get_left_button()->set_frame_style( 0, style );
        h_slider->get_right_button()->set_frame_style( 0, style );

        style = v_slider->get_thumb_button()->get_frame_style( 1 );
        style.set_color( 0.7, 0.7, 0.7, 1.0 );

        v_slider->get_thumb_button()->set_frame_style( 1, style );
        v_slider->get_left_button()->set_frame_style( 1, style );
        v_slider->get_right_button()->set_frame_style( 1, style );

        h_slider->get_thumb_button()->set_frame_style( 1, style );
        h_slider->get_left_button()->set_frame_style( 1, style );
        h_slider->get_right_button()->set_frame_style( 1, style );

        style = v_slider->get_thumb_button()->get_frame_style( 2 );
        style.set_color( 1.0, 1.0, 1.0, 1.0 );

        v_slider->get_thumb_button()->set_frame_style( 2, style );
        v_slider->get_left_button()->set_frame_style( 2, style );
        v_slider->get_right_button()->set_frame_style( 2, style );

        h_slider->get_thumb_button()->set_frame_style( 2, style );
        h_slider->get_left_button()->set_frame_style( 2, style );
        h_slider->get_right_button()->set_frame_style( 2, style );

        frame_np.reparent_to( schemenode );
}

static void make_text_entry( Object &pobj, Parser &parser, const NodePath &schemenode )
{
        PT( PPGuiEntry ) entry = new PPGuiEntry( "" );

        PT( TextNode ) text_def = new TextNode( "ppguientry_textdef" );
        text_def->set_align( TextNode::A_left );
        text_def->set_shadow( .04, .04 );
        text_def->set_shadow_color( LColor( 0, 0, 0, 0 ) );

        entry->set_text_def( 0, text_def );
        entry->set_text_def( 1, text_def );
        entry->set_text_def( 2, text_def );
        entry->set_text_def( 3, text_def );

        NodePath entry_np( entry );
        apply_basic_props( parser, pobj, entry_np );

        for ( size_t i = 0; i < pobj.properties.size(); i++ )
        {
                Property prop = pobj.properties[i];

                if ( prop.name == "numlines" )
                {
                        entry->set_num_lines( stoi( prop.value ) );
                }

                else if ( prop.name == "overflowmode" )
                {
                        entry->set_overflow_mode( bool_from_str( prop.value ) );
                }

                else if ( prop.name == "blinkrate" )
                {
                        entry->set_blink_rate( stof( prop.value ) );
                }

                else if ( prop.name == "maxchars" )
                {
                        entry->set_max_chars( stoi( prop.value ) );
                }

                else if ( prop.name == "maxwidth" )
                {
                        entry->set_max_width( stof( prop.value ) );
                }

                else if ( prop.name == "obscuremode" )
                {
                        entry->set_obscure_mode( bool_from_str( prop.value ) );
                }

                else if ( prop.name == "cursorkeys" )
                {
                        entry->set_cursor_keys_active( bool_from_str( prop.value ) );
                }

                else if ( prop.name == "focus" )
                {
                        entry->set_focus( bool_from_str( prop.value ) );
                }

                else if ( prop.name == "backgroundfocus" )
                {
                        entry->set_background_focus( bool_from_str( prop.value ) );
                }

                else if ( prop.name == "initialtext" )
                {
                        entry->set_text( prop.value );
                        entry->set_cursor_position( entry->get_num_characters() );
                }

                else if ( prop.name == "textfont" )
                {
                        text_def->set_font( Gui::get_font( prop.value ) );
                }

                else if ( prop.name == "textcolor" )
                {
                        vector<PN_stdfloat> col = Parser::parse_float_list_str( prop.value );
                        text_def->set_text_color( col[0], col[1], col[2], col[3] );
                }

                else if ( prop.name == "textshadow" )
                {
                        vector<PN_stdfloat> sh = Parser::parse_float_list_str( prop.value );
                        text_def->set_shadow_color( sh[0], sh[1], sh[2], sh[3] );
                }
        }

        entry->setup( entry->get_max_width(), entry->get_num_lines() );

        entry_np.reparent_to( schemenode );
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

                        if ( pobj.name == "label")
                        {
                                // This scheme contains a label.
                                make_label( pobj, parser, schemenode );
                        }

                        else if ( pobj.name == "image" )
                        {
                                // This is an image.
                                make_image( pobj, parser, schemenode );
                        }
                        else if ( pobj.name == "button" )
                        {
                                // This is a good ole button.
                                make_button( pobj, parser, schemenode );
                        }
                        else if ( pobj.name == "scrollFrame" )
                        {
                                // A scrolled frame.
                                make_scrolled_frame( pobj, parser, schemenode );
                        }
                        else if ( pobj.name == "entry" )
                        {
                                // It's a text entry box.
                                make_text_entry( pobj, parser, schemenode );
                        }
                }
                walk_schemes( parser, parser.get_objects_with_name( obj, "scheme" ), scheme );
        }
}

NodePath Gui::load_scheme( const string &filename )
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

                        PT( DynamicTextFont ) font = PPUtils::load_dynamic_font( path );

                        if ( parser.has_property( obj, "spacewidth" ) )
                        {
                                PN_stdfloat spw = stof( parser.get_property_value( obj, "spacewidth" ) );
                                font->set_space_advance( spw );
                        }

                        if ( parser.has_property( obj, "lineheight" ) )
                        {
                                PN_stdfloat lih = stof( parser.get_property_value( obj, "lineheight" ) );
                                font->set_line_height( lih );
                        }

                        if ( parser.has_property( obj, "quality" ) )
                        {
                                int quality = stoi( parser.get_property_value( obj, "quality" ) );
                                font->set_pixels_per_unit( quality );
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
                                PN_stdfloat vol = stof( parser.get_property_value( obj, "volume" ) );
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
                parser.make_objects_global();
                return NodePath();
        }

        PT( GuiScheme ) base_scheme = new GuiScheme( "baseScheme" );

        // Walk through every single Object and create the GUI schemes and elements.
        walk_schemes( parser, base_schemes, base_scheme );

        return NodePath( base_scheme );
}