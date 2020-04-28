/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file phys_material.h
 * @author Brian Lach
 * @date April 27, 2020
 */

#pragma once

#include <referenceCount.h>

#include "config_bphysics.h"
#include "physx_types.h"

class EXPORT_BPHYSICS CPhysMaterial : public ReferenceCount
{
PUBLISHED:
	enum Flags
	{
		F_disable_friction		= 1 << 0,
		F_disable_strong_friction	= 1 << 1,
		F_improved_patch_friction	= 1 << 2,
	};

	enum CombineMode
	{
		C_average	= 0,
		C_min		= 1,
		C_multiply	= 2,
		C_max		= 3,
	};

	CPhysMaterial( float flStaticFriction, float flDynamicFriction, float flRestitution );
	~CPhysMaterial();

	void set_static_friction( float flStaticFriction );
	float get_static_friction() const;

	void set_dynamic_friction( float flDynamicFriction );
	float get_dynamic_friction() const;

	void set_restitution( float flRestitution );
	float get_restitution() const;

	void set_flag( int flag, bool bSleeping );
	int get_flags() const;

	void set_friction_combine_mode( CombineMode mode );
	CombineMode get_friction_combine_mode() const;

	void set_restitution_combine_mode( CombineMode mode );
	CombineMode get_resititution_combine_mode() const;

public:
	PxMaterial *get_material() const;

private:
	PxMaterial *m_pMaterial;
};

INLINE PxMaterial *CPhysMaterial::get_material() const
{
	return m_pMaterial;
}
