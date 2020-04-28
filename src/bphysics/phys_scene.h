/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file phys_scene.h
 * @author Brian Lach
 * @date April 27, 2020
 */

#pragma once

#include "config_bphysics.h"
#include <referenceCount.h>
#include "physx_types.h"

#include "phys_scene_desc.h"

class CPhysScene : public ReferenceCount
{
PUBLISHED:
	CPhysScene( const CPhysSceneDesc &desc );
	~CPhysScene();

private:
	PxScene *m_pScene;
};