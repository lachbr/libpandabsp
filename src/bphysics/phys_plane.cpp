/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file phys_plane.cpp
 * @author Brian Lach
 * @date April 28, 2020
 */

#include "phys_plane.h"
#include "physx_globals.h"

CPhysPlane::CPhysPlane() :
	CPhysGeometry()
{
	m_pGeometry = new PxPlaneGeometry;
}