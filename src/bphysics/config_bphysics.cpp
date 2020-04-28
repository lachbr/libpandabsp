/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file config_bphysics.cpp
 * @author Brian Lach
 * @date April 27, 2020
 */

#include "config_bphysics.h"
#include "physx_globals.h"

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

	GetPxDefaultAllocator();
	GetPxDefaultErrorCallback();
	GetPxDefaultCpuDispatcher();
	GetPxFoundation();
	GetPxPhysics();
}
