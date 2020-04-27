#include "phys_material.h"
#include "physx_globals.h"

CPhysMaterial::CPhysMaterial( float flStaticFriction, float flDynamicFriction, float flRestitution )
{
	m_pMaterial = GetPxPhysics()->createMaterial( flStaticFriction, flDynamicFriction, flRestitution );
}