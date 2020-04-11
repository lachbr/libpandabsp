#pragma once

#include "baseentity.h"

class CToonCage : public CBaseEntity
{
	DECLARE_ENTITY( CToonCage, CBaseEntity )
public:
	virtual void add_components();
	virtual void spawn();
};