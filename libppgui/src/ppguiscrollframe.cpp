#include "ppGuiScrollFrame.h"

TypeDef( PPGuiScrollFrame );

PPGuiScrollFrame::
PPGuiScrollFrame( const string &name ) :
        PGScrollFrame( name )
{
}

NodePath &PPGuiScrollFrame::
get_canvas()
{
        return _canvas_np;
}

void PPGuiScrollFrame::
setup( PN_stdfloat width, PN_stdfloat height,
       PN_stdfloat left, PN_stdfloat right, PN_stdfloat bottom, PN_stdfloat top,
       PN_stdfloat slider_width, PN_stdfloat bevel )
{
        PGScrollFrame::setup( width, height,
                              left, right, bottom, top,
                              slider_width, bevel );

        _canvas_np = NodePath( get_canvas_node() );
}