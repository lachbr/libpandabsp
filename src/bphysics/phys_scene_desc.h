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
