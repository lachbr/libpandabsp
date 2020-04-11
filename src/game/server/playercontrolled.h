#pragma once

#include "basecomponent.h"

class CUserCmd;

class CPlayerControlled : public CBaseComponent
{
	DECLARE_CLASS( CPlayerControlled, CBaseComponent )
public:
	virtual void player_run_command( CUserCmd *cmd, float frametime )
	{
	}
};