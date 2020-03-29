#include "config_networksystem.h"

#include <steam/steamnetworkingsockets.h>

NotifyCategoryDef( networksystem, "" )

ConfigureDef( config_networksystem )

ConfigureFn( config_networksystem )
{
	init_libnetworksystem();
}

void init_libnetworksystem()
{
	static bool initialized = false;
	if ( initialized )
		return;

	// Initialize the network library
	SteamNetworkingErrMsg err;
	if ( !GameNetworkingSockets_Init( nullptr, err ) )
	{
		networksystem_cat.fatal()
			<< "Unable to initialize GameNetworkingSockets!\n";
	}

	initialized = true;
}