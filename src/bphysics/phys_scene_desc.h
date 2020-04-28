/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file phys_scene_desc.h
 * @author Brian Lach
 * @date April 27, 2020
 */

#pragma once

#include "config_bphysics.h"
#include <PxSceneDesc.h>
#include "luse.h"

class EXPORT_BPHYSICS CPhysSceneDesc
{
PUBLISHED:
	CPhysSceneDesc();

	void set_gravity( const LVector3f &gravity );
	LVector3f get_gravity() const;

public:
	const physx::PxSceneDesc &get_desc() const;

private:
	physx::PxSceneDesc m_Desc;
};
