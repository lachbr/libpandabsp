#ifndef __GUISCHEME_H__
#define __GUISCHEME_H__

#include "config_ppgui.h"
#include "pp_utils.h"

#include <pgItem.h>

class EXPCL_PPGUI GuiScheme : public PGItem
{
        TypeDecl( GuiScheme, PGItem );
public:
        GuiScheme( const string &name );
        virtual ~GuiScheme();
};

#endif // __GUISCHEME_H__