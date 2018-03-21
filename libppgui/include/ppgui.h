#ifndef __GUI_H__
#define __GUI_H__

#include "config_ppgui.h"

#include <pointerTo.h>
#include <textFont.h>
#include <audioSound.h>
#include <texture.h>

class GuiScheme;

struct FontDef
{
        string name;
        PT( TextFont ) font;
};

struct SoundDef
{
        string name;
        PT( AudioSound ) sound;
};

struct TextureDef
{
        string name;
        PT( Texture ) tex;
};

class EXPCL_PPGUI Gui
{
public:

        /**
        * Should return a GuiScheme object (which inherits from NodePath,
        *                                   it's kind of like a DirectFrame from the Python level).
        * The GuiScheme should be a hierarchial class which contains other child GuiSchemes.
        * Each GuiScheme contains the physical GUI elements that the programmer can access and
        * modify via code, as well as other child GuiSchemes.
        * The programmer can access the other schemes and gui elements by doing:
        *
        * scheme->get_element("yesbutton");
        * scheme->get_child_scheme("infodialog");
        *
        * Every scheme should be defined in a .vif file (Valve Information Format). It uses Valve's
        * syntax for their key-value hierarchial data.
        */
        static NodePath load_scheme( const string &file_path ); // The void * will become a PT(GuiScheme).
        // If I decide to reference count it.

        static pvector<FontDef> _fonts;
        static pvector<SoundDef> _sounds;
        static pvector<TextureDef> _textures;

        static void store_sound( const string &name, PT( AudioSound ) sound );
        static void store_font( const string &name, PT( TextFont ) font );
        static void store_texture( const string &name, PT( Texture ) tex );

        static PT( AudioSound ) get_sound( const string &name );
        static PT( Texture ) get_texture( const string &name );
        static PT( TextFont ) get_font( const string &name );

};

#endif // __GUI_H__