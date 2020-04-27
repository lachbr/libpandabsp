#include "physx_globals.h"

physx::PxFoundation *GetPxFoundation()
{
	static physx::PxFoundation *pFoundation = physx::PxCreateFoundation( PX_PHYSICS_VERSION, GetPxDefaultAllocator(), GetPxDefaultErrorCallback() );
	return pFoundation;
}

physx::PxPhysics *GetPxPhysics()
{
	static physx::PxPhysics *pPhysics = physx::PxCreatePhysics( PX_PHYSICS_VERSION, *GetPxFoundation(), physx::PxTolerancesScale(), true, nullptr );
	return pPhysics;
}
