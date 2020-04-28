/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file physx_globals.cpp
 * @author Brian Lach
 * @date April 27, 2020
 */

#include "physx_globals.h"

PxFoundation *GetPxFoundation()
{
	static PxFoundation *pFoundation = PxCreateFoundation( PX_PHYSICS_VERSION, *GetPxDefaultAllocator(), *GetPxDefaultErrorCallback() );
	return pFoundation;
}

PxPhysics *GetPxPhysics()
{
	static PxPhysics *pPhysics = PxCreatePhysics( PX_PHYSICS_VERSION, *GetPxFoundation(), physx::PxTolerancesScale(), true );
	return pPhysics;
}

PxDefaultCpuDispatcher *GetPxDefaultCpuDispatcher()
{
	static PxDefaultCpuDispatcher *pDispatch = PxDefaultCpuDispatcherCreate( 1 );
	return pDispatch;
}

PxDefaultErrorCallback *GetPxDefaultErrorCallback()
{
	static PxDefaultErrorCallback callback;
	return &callback;
}

PxDefaultAllocator *GetPxDefaultAllocator()
{
	static PxDefaultAllocator alloc;
	return &alloc;
}
