#pragma once

#include "c_baseanimating.h"

class EXPORT_CLIENT_DLL C_BaseCombatCharacter : public C_BaseAnimating
{
	DECLARE_CLIENTCLASS( C_BaseCombatCharacter, C_BaseAnimating )

public:
	C_BaseCombatCharacter();

	INLINE int get_max_health() const
	{
		return _max_health;
	}

	INLINE int get_health() const
	{
		return _health;
	}

private:
	int _max_health;
	int _health;
};
