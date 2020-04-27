#include "config_bphysics.h"

ConfigureDef( config_bphysics )

ConfigureFn( config_bphysics )
{
	init_bphysics();
}

void init_bphysics()
{
	static bool s_bInitialized = false;
	if ( s_bInitialized )
	{
		return;
	}

	s_bInitialized = true;
}