#pragma once

#include "config_clientdll.h"
#include "c_basecomponent.h"

class CUserCmd;

class EXPORT_CLIENT_DLL C_PlayerControlled : public C_BaseComponent
{
	DECLARE_CLASS( C_PlayerControlled, C_BaseComponent )
public:
	virtual void player_run_command( CUserCmd *cmd, float frametime )
	{
	}
};