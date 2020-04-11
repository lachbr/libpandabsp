#pragma once

#include "c_basecomponent.h"

class EXPORT_CLIENT_DLL C_BaseCombat : public C_BaseComponent
{
	DECLARE_CLIENTCLASS( C_BaseCombat, C_BaseComponent )

public:
	C_BaseCombat();

	INLINE int get_max_health() const
	{
		return max_health;
	}

	INLINE int get_health() const
	{
		return health;
	}

private:
	int max_health;
	int health;
};
