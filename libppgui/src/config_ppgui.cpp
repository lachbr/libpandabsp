#include "config_ppgui.h"

#include "ppguiimage.h"
#include "ppguilabel.h"
#include "ppguischeme.h"
#include "ppguibutton.h"
#include "ppguiscrollframe.h"
#include "ppguientry.h"

ConfigureDef( config_ppGui );
ConfigureFn( config_ppGui )
{
        init_libppgui();
}

void init_libppgui()
{
        static bool initialized = false;
        if ( initialized )
        {
                return;
        }
        initialized = true;

        GuiImage::init_type();
        GuiLabel::init_type();
        GuiScheme::init_type();
        PPGuiButton::init_type();
        PPGuiScrollFrame::init_type();
        PPGuiEntry::init_type();
}