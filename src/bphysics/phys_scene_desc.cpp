/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file phys_scene_desc.cpp
 * @author Brian Lach
 * @date April 27, 2020
 */

#include "phys_scene_desc.h"
#include "physx_utils.h"

CPhysSceneDesc::CPhysSceneDesc() :
	m_Desc( physx::PxTolerancesScale() )
{
}

void CPhysSceneDesc::set_gravity( const LVector3f &gravity )
{
	m_Desc.gravity = Vec3_to_PxVec3( gravity );
}

LVector3f CPhysSceneDesc::get_gravity() const
{
	return PxVec3_to_Vec3( m_Desc.gravity );
}

const physx::PxSceneDesc &CPhysSceneDesc::get_desc() const
{
	return m_Desc;
}
