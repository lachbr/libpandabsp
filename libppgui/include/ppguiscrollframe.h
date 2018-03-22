#ifndef PP_GUI_SCROLL_BAR_H
#define PP_GUI_SCROLL_BAR_H

#include "config_ppgui.h"
#include "pp_utils.h"

#include <pgScrollFrame.h>
#include <nodePath.h>

class EXPCL_PPGUI PPGuiScrollFrame : public PGScrollFrame
{
public:
        PPGuiScrollFrame( const string &name = "" );

        void setup( PN_stdfloat width, PN_stdfloat height,
                    PN_stdfloat left, PN_stdfloat right, PN_stdfloat bottom, PN_stdfloat top,
                    PN_stdfloat slider_width, PN_stdfloat bevel );

        NodePath &get_canvas();
private:
        NodePath _canvas_np;

        TypeDecl( PPGuiScrollFrame, PGScrollFrame );
};

#endif // PP_GUI_SCROLL_BAR_H