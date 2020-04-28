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
