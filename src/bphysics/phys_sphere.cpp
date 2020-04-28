/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file phys_sphere.cpp
 * @author Brian Lach
 * @date April 28, 2020
 */

#include "phys_sphere.h"
#include "physx_globals.h"

CPhysSphere::CPhysSphere() :
	CPhysGeometry()
{
	m_pGeometry = new PxSphereGeometry;
}

CPhysSphere::CPhysSphere( float flRadius ) :
	CPhysGeometry()
{
	m_pGeometry = new PxSphereGeometry( flRadius );
}

bool CPhysSphere::is_valid() const
{
	return ( (PxSphereGeometry *)m_pGeometry )->isValid();
}

float CPhysSphere::get_radius() const
{
	return ( (PxSphereGeometry *)m_pGeometry )->radius;
}
