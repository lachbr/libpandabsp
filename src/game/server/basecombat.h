#pragma once

#include "basecomponent.h"

class EXPORT_SERVER_DLL CBaseCombat : public CBaseComponent
{
	DECLARE_SERVERCLASS( CBaseCombat, CBaseComponent )

public:
	CBaseCombat();

	virtual bool initialize();

public:
	NetworkInt( health );
	NetworkInt( max_health );
};