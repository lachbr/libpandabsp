/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file phys_scene.cpp
 * @author Brian Lach
 * @date April 27, 2020
 */

#include "phys_scene.h"
#include "physx_globals.h"

CPhysScene::CPhysScene( const CPhysSceneDesc &desc )
{
	m_pScene = GetPxPhysics()->createScene( desc.get_desc() );
}

CPhysScene::~CPhysScene()
{
	if ( m_pScene )
	{
		m_pScene->release();
		m_pScene = nullptr;
	}
}
