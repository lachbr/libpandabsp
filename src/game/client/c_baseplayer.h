#pragma once

#include "c_basecombatcharacter.h"
#include "baseplayer_shared.h"

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

class EXPORT_CLIENT_DLL C_BasePlayer : public C_BaseCombatCharacter, public CBasePlayerShared
{
	DECLARE_CLIENTCLASS( C_BasePlayer, C_BaseCombatCharacter )

public:
	C_BasePlayer();

	virtual void setup_controller();
	virtual void spawn();

	virtual void simulate();

	bool is_local_player() const;

	C_CommandContext *get_command_context();

protected:
	LVector3f _view_offset;
	CInterpolatedVar<LVector3f> _iv_view_offset;

	C_CommandContext _cmd_ctx;
	int _tickbase;
};

INLINE C_CommandContext *C_BasePlayer::get_command_context()
{
	return &_cmd_ctx;
}