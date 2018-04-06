#ifndef DIRECTGUI_WIDGET_H
#define DIRECTGUI_WIDGET_H

#include "config_directgui.h"

#include <nodePath.h>
#include <pgFrameStyle.h>
#include <pgItem.h>

struct DirectGuiOptions
{
	int num_states;
	int sort_order;
};

class EXPCL_DIRECTGUI DirectGuiWidget : public NodePath
{
public:
	enum State
	{
		S_disabled,
		S_normal
	};

	enum Relief
	{
		R_none = PGFrameStyle::T_none,
		R_flat = PGFrameStyle::T_flat,
		R_rasied = PGFrameStyle::T_bevel_out,
		R_sunken = PGFrameStyle::T_bevel_in,
		R_groove = PGFrameStyle::T_groove,
		R_ridge = PGFrameStyle::T_ridge,
		R_texture_border = PGFrameStyle::T_texture_border,
	};

	DirectGuiWidget();

	virtual void initialize_options();
	virtual void setup();
        virtual void destroy();

        string get_gui_id() const;

	void set_state( int state );
	void set_relief( int relief );
	void set_border_width( const LVector2f &width );
	void set_border_uv_width( const LVector2f &width );
	void set_frame_size( const LVector4f &size, bool clear_frame = false );
	void set_frame_color( const LColorf &color );
	void set_frame_texture( PT( Texture ) tex );
        void set_frame_texture( const pvector<PT( Texture )> &texs );
	void set_frame_visible_scale( const LVector2f &scale );
	void reset_frame_size();
        void update_frame_style();

        PN_stdfloat get_width() const;
        PN_stdfloat get_height() const;
        LVector2f get_center() const;
	LVector4f get_bounds( int state = 0 );
	PGFrameStyle::Type get_frame_type( int state = 0 ) const;

protected:
	PT( PGItem ) _gui_item;

	int _num_states;
	int _sort_order;
	int _state;
	int _relief;
	LVector2f _border_width;
	LVector2f _border_uv_width;
	LVector4f _frame_size;
	bool _auto_frame_size;
	LColorf _frame_color;
        pvector<PT( Texture )> _frame_texture;
	LVector2f _frame_visible_scale;
	LVector2f _pad;
	bool _suppress_mouse;
	bool _suppress_keys;

	// For holding bounds info
	LPoint3f _ll;
	LPoint3f _ur;
	LVector4f _bounds;

	pvector<PGFrameStyle> _frame_style;
	pvector<NodePath> _state_nodepath;

	bool _f_init;
};

#endif // DIRECTGUI_WIDGET_H