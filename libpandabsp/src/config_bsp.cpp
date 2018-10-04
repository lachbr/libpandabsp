#include "config_bsp.h"
#include "entity.h"
#include "bsploader.h"
#include "bsp_render.h"

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

	BSPFaceAttrib::init_type();
        //WorldGeometryAttrib::init_type();
        IgnorePVSAttrib::init_type();
	CBaseEntity::init_type();
        CPointEntity::init_type();
	CBrushEntity::init_type();
        CBoundsEntity::init_type();
        BSPRender::init_type();
        BSPCullTraverser::init_type();
        BSPRoot::init_type();
        BSPProp::init_type();
        BSPModel::init_type();
}