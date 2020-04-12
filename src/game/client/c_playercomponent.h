#pragma once

#include "c_basecomponent.h"
#include "usercmd.h"
#include "client_commandmgr.h"

#include "luse.h"

class C_CommandContext
{
public:
	bool needsprocessing;

	CUserCmd cmd;
	int command_number;
};

class C_PredictionError
{
public:
	float time;
	LVector3 error;
};

class EXPORT_CLIENT_DLL C_PlayerComponent : public C_BaseComponent
{
	DECLARE_CLIENTCLASS( C_PlayerComponent, C_BaseComponent )
public:
	C_PlayerComponent();

	//virtual bool initialize();
	virtual void update( double frametime );
	virtual void spawn();

	virtual void create_cmd( CUserCmd *cmd )
	{
	}

	virtual void setup_view( CUserCmd *cmd )
	{
	}

	virtual void player_run_command( CUserCmd *cmd, float frametime )
	{
	}

	C_CommandContext *get_command_context();

	int tickbase;
	int simulation_tick;
	LVector3f view_offset;
	CInterpolatedVar<LVector3f> iv_view_offset;
	C_CommandContext _cmd_ctx;

	CClientCMDManager _cmd_mgr;
};

inline C_CommandContext *C_PlayerComponent::get_command_context()
{
	return &_cmd_ctx;
}