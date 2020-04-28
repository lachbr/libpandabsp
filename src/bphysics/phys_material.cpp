#include "phys_material.h"
#include "physx_globals.h"

CPhysMaterial::CPhysMaterial( float flStaticFriction, float flDynamicFriction, float flRestitution )
{
	m_pMaterial = GetPxPhysics()->createMaterial( flStaticFriction, flDynamicFriction, flRestitution );
}

CPhysMaterial::~CPhysMaterial()
{
	if ( m_pMaterial )
	{
		m_pMaterial->release();
		m_pMaterial = nullptr;
	}
}

void CPhysMaterial::set_static_friction( float flStaticFriction )
{
	m_pMaterial->setStaticFriction( flStaticFriction );
}

float CPhysMaterial::get_static_friction() const
{
	return m_pMaterial->getStaticFriction();
}

void CPhysMaterial::set_dynamic_friction( float flDynamicFriction )
{
	m_pMaterial->setDynamicFriction( flDynamicFriction );
}

float CPhysMaterial::get_dynamic_friction() const
{
	return m_pMaterial->getDynamicFriction();
}

void CPhysMaterial::set_restitution( float flRestitution )
{
	m_pMaterial->setRestitution( flRestitution );
}

float CPhysMaterial::get_restitution() const
{
	return m_pMaterial->getRestitution();
}
