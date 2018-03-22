#ifndef PPGUIBUTTON_H
#define PPGUIBUTTON_H

#include <pgButton.h>

#include "config_ppgui.h"
#include "pp_utils.h"

class EXPCL_PPGUI PPGuiButton : public PGButton
{
        TypeDecl( PPGuiButton, PGButton );

public:
        PPGuiButton( const string &name );

        virtual void press( const MouseWatcherParameter &param, bool background );

        void set_text( const string &text );

        void set_rollover( PT( Texture ) tex );
        void set_idle( PT( Texture ) tex );
        void set_down( PT( Texture ) tex );
        void set_disabled( PT( Texture ) tex );

        void set_click_sound( PT( AudioSound ) s );
        void set_rollover_sound( PT( AudioSound ) s );

        void set_fit_to_text( bool flag );
        void set_geom_offset( LVector4f &offset );

        void set_text_color( LColorf &color );

        void set_text0_color( LColorf &color );
        void set_text1_color( LColorf &color );
        void set_text2_color( LColorf &color );
        void set_text3_color( LColorf &color );

        void set_text_pos( const LPoint3f &pos );
        void set_text0_pos( const LPoint3f &pos );
        void set_text1_pos( const LPoint3f &pos );
        void set_text2_pos( const LPoint3f &pos );
        void set_text3_pos( const LPoint3f &pos );

        void set_text_only( bool flag );
        bool get_text_only() const;

        void set_geom_scale( LVector3f &scale );

        TextNode *get_text();

        void setup();

        void set_click_event( const string &event );

private:
        NodePath make_card( PT( Texture ) tex );
        void setup_text();

        void attach_text_to_state_nodes();
        void remove_text_from_state_nodes();

private:
        bool _setup;

        bool _fit_to_text;

        bool _text_only;

        PT( Texture ) _idle_tex;
        PT( Texture ) _rlvr_tex;
        PT( Texture ) _down_tex;
        PT( Texture ) _disabled_tex;

        PT( AudioSound ) _sound_click;
        PT( AudioSound ) _sound_rlvr;

        LVector3f _geom_scale;
        LVector4f _geom_offset;

        PT( TextNode ) _text;

        NodePath _text_np;

        NodePath _idle_np;
        NodePath _down_np;
        NodePath _rlvr_np;
        NodePath _inac_np;

        LColorf _text0_color;
        LColorf _text1_color;
        LColorf _text2_color;
        LColorf _text3_color;

        LPoint3f _text0_pos;
        LPoint3f _text1_pos;
        LPoint3f _text2_pos;
        LPoint3f _text3_pos;

        string _click_event;
};

#endif // PPGUIBUTTON_H