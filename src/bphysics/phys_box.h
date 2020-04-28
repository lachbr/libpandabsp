/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file phys_box.h
 * @author Brian Lach
 * @date April 28, 2020
 */

#pragma once

#include "config_bphysics.h"
#include "phys_geometry.h"
#include "luse.h"

class EXPORT_BPHYSICS CPhysBox : public CPhysGeometry
{
PUBLISHED:
	CPhysBox();
	CPhysBox( const LVector3f &halfExtents );
};
