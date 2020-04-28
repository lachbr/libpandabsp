/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file phys_sphere.h
 * @author Brian Lach
 * @date April 28, 2020
 */

#pragma once

#include "config_bphysics.h"
#include "phys_geometry.h"

class EXPORT_BPHYSICS CPhysSphere : public CPhysGeometry
{
PUBLISHED:
	CPhysSphere();
	CPhysSphere( float flRadius );

	float get_radius() const;

	bool is_valid() const;
};

