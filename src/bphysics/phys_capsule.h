/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file phys_capsule.h
 * @author Brian Lach
 * @date April 28, 2020
 */

#pragma once

#include "config_bphysics.h"
#include "phys_geometry.h"

class EXPORT_BPHYSICS CPhysCapsule : public CPhysGeometry
{
PUBLISHED:
	CPhysCapsule();
	CPhysCapsule( float flRadius, float flHalfHeight );
};