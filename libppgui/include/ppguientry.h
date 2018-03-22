#include <pgEntry.h>

#include "config_ppgui.h"
#include "pp_utils.h"

class EXPCL_PPGUI PPGuiEntry : public PGEntry
{
        TypeDecl( PPGuiEntry, PGEntry );

public:
        PPGuiEntry( const string &name );
        //virtual ~PPGuiEntry();
};