#include "config_bsp.h"

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

}