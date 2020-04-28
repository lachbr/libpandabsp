/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file phys_capsule.cpp
 * @author Brian Lach
 * @date April 28, 2020
 */

#include "phys_capsule.h"
#include "physx_globals.h"

CPhysCapsule::CPhysCapsule() :
	CPhysGeometry()
{
	m_pGeometry = new PxCapsuleGeometry;
}

CPhysCapsule::CPhysCapsule( float flRadius, float flHalfHeight ) :
	CPhysGeometry()
{
	m_pGeometry = new PxCapsuleGeometry( flRadius, flHalfHeight );
}
