#ifndef __GUILABEL_H__
#define __GUILABEL_H__

#include "config_ppgui.h"
//#include "ppGuiItem.h"

#include <textNode.h>

#include <pp_utils.h>

class EXPCL_PPGUI GuiLabel : public TextNode
{
public:
        GuiLabel( const string &name );

        TypeDecl( GuiLabel, TextNode );
};

#endif // __GUILABEL_H__