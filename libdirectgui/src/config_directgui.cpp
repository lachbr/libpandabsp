#include "config_directgui.h"

ConfigureDef( libdirectgui );
ConfigureFn( libdirectgui )
{
	init_libdirectgui();
}

void init_libdirectgui()
{
	static bool initialized = false;
	if ( initialized )
	{
		return;
	}
	initialized = true;


}