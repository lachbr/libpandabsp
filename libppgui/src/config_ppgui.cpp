#include "config_ppgui.h"

#include "ppGuiImage.h"
#include "ppGuiLabel.h"
#include "ppGuiScheme.h"
#include "ppGuiButton.h"
#include "ppGuiScrollFrame.h"
#include "ppGuiEntry.h"

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