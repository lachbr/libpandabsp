/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file phys_box.cpp
 * @author Brian Lach
 * @date April 28, 2020
 */

#include "phys_box.h"
#include "physx_globals.h"

CPhysBox::CPhysBox() :
	CPhysGeometry()
{
	m_pGeometry = new PxBoxGeometry;
}

CPhysBox::CPhysBox( const LVector3f &halfExtents ) :
	CPhysGeometry()
{
	m_pGeometry = new PxBoxGeometry( halfExtents[0], halfExtents[1], halfExtents[2] );
}
