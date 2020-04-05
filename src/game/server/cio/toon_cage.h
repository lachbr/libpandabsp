#pragma once

#include "baseanimating.h"

class CToonCage : public CBaseAnimating
{
	DECLARE_SERVERCLASS( CToonCage, CBaseAnimating )
public:
	virtual void spawn();
};