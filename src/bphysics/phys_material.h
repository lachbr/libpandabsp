#pragma once

#include <referenceCount.h>
#include "physx_types.h"

class CPhysMaterial : public ReferenceCount
{
PUBLISHED:
	CPhysMaterial( float flStaticFriction, float flDynamicFriction, float flRestitution );
	~CPhysMaterial();

	void set_static_friction( float flStaticFriction );
	float get_static_friction() const;

	void set_dynamic_friction( float flDynamicFriction );
	float get_dynamic_friction() const;

	void set_restitution( float flRestitution );
	float get_restitution() const;

private:
	PxMaterial *m_pMaterial;
};
