/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file phys_material.cpp
 * @author Brian Lach
 * @date April 27, 2020
 */

#include "phys_material.h"
#include "physx_globals.h"

CPhysMaterial::CPhysMaterial( float flStaticFriction, float flDynamicFriction, float flRestitution )
{
	m_pMaterial = GetPxPhysics()->createMaterial( flStaticFriction, flDynamicFriction, flRestitution );
	m_pMaterial->userData = this;
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

void CPhysMaterial::set_flag( int flag, bool bSleeping )
{
	m_pMaterial->setFlag( (PxMaterialFlag::Enum)flag, bSleeping );
}

int CPhysMaterial::get_flags() const
{
	return (uint16_t)m_pMaterial->getFlags();
}

void CPhysMaterial::set_friction_combine_mode( CPhysMaterial::CombineMode mode )
{
	m_pMaterial->setFrictionCombineMode( ( PxCombineMode::Enum )mode );
}

CPhysMaterial::CombineMode CPhysMaterial::get_friction_combine_mode() const
{
	return (CombineMode)m_pMaterial->getFrictionCombineMode();
}

void CPhysMaterial::set_restitution_combine_mode( CPhysMaterial::CombineMode mode )
{
	m_pMaterial->setRestitutionCombineMode( ( PxCombineMode::Enum )mode );
}

CPhysMaterial::CombineMode CPhysMaterial::get_resititution_combine_mode() const
{
	return (CombineMode)m_pMaterial->getRestitutionCombineMode();
}
