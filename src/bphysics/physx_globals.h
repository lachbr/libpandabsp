#pragma once

#include <PxFoundation.h>
#include <PxPhysics.h>
#include <PxPhysicsAPI.h>
#include <PxMaterial.h>
#include <extensions/PxDefaultCpuDispatcher.h>
#include <extensions/PxDefaultAllocator.h>
#include <extensions/PxDefaultErrorCallback.h>

using namespace physx;

extern physx::PxFoundation *GetPxFoundation();
extern physx::PxPhysics *GetPxPhysics();
extern physx::PxDefaultCpuDispatcher *GetPxDefaultCpuDispatcher();
extern physx::PxDefaultAllocator *GetPxDefaultAllocator();
extern physx::PxDefaultErrorCallback *GetPxDefaultErrorCallback();
