/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file phys_geometry.h
 * @author Brian Lach
 * @date April 28, 2020
 */

#pragma once

#include "config_bphysics.h"
#include <referenceCount.h>
#include "physx_types.h"

class EXPORT_BPHYSICS CPhysGeometry : public ReferenceCount
{
PUBLISHED:
	CPhysGeometry();
	virtual ~CPhysGeometry();

	PxGeometry *get_geometry() const;

protected:
	PxGeometry *m_pGeometry;
};

INLINE PxGeometry *CPhysGeometry::get_geometry() const
{
	return m_pGeometry;
}