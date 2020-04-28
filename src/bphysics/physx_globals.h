/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file physx_globals.h
 * @author Brian Lach
 * @date April 27, 2020
 */

#pragma once

#if !defined(NDEBUG) && !defined(_DEBUG)
#define NDEBUG 1
#include <PxPhysicsAPI.h>
#undef NDEBUG
#else
#include <PxPhysicsAPI.h>
#endif

using namespace physx;

extern PxFoundation *GetPxFoundation();
extern PxPhysics *GetPxPhysics();
extern PxDefaultCpuDispatcher *GetPxDefaultCpuDispatcher();
extern PxDefaultAllocator *GetPxDefaultAllocator();
extern PxDefaultErrorCallback *GetPxDefaultErrorCallback();
