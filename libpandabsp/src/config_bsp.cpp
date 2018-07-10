#include "config_bsp.h"
#include "entity.h"
#include "bsploader.h"

ConfigureDef( config_bsp );
ConfigureFn( config_bsp )
{
        init_libpandabsp();
}

void init_libpandabsp()
{
        static bool initialized = false;
        if ( initialized )
                return;
        initialized = true;

        BSPGeomNode::init_type();
	BSPFaceAttrib::init_type();
	CBaseEntity::init_type();
	CBrushEntity::init_type();

}