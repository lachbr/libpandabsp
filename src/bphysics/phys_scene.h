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