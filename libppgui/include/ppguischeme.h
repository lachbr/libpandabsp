#ifndef __GUISCHEME_H__
#define __GUISCHEME_H__

//#include <referenceCount.h>
//#include "ppGuiItem.h"
#include "config_ppgui.h"
#include <pandaNode.h>

#include <pp_utils.h>

#include <pgItem.h>

class EXPCL_PPGUI GuiScheme : public PGItem
{
public:
        GuiScheme( const string &name );
        virtual ~GuiScheme();

        //void add_child_scheme(PT(GuiScheme) scheme);
        //void add_element(GuiItem *item);

        //GuiScheme *get_child_scheme(const string &name);
        //GuiItem *get_element(const string &name);

        TypeDecl( GuiScheme, PGItem );
};

#endif // __GUISCHEME_H__