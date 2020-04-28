/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file phys_geometry.cpp
 * @author Brian Lach
 * @date April 28, 2020
 */

#include "phys_geometry.h"

CPhysGeometry::CPhysGeometry()
{
	m_pGeometry = nullptr;
}

CPhysGeometry::~CPhysGeometry()
{
	if ( m_pGeometry )
	{
		delete m_pGeometry;
		m_pGeometry = nullptr;
	}
}
