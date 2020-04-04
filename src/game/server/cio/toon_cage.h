#pragma once

#include "baseanimating.h"

class CToonCage : public CBaseAnimating
{
	DECLARE_CLASS( CToonCage, CBaseAnimating )
	DECLARE_SERVERCLASS()
public:
	virtual void spawn();
};