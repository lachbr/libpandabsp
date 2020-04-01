#pragma once

#include "config_game_shared.h"
#include "physics_character_controller.h"
#include "usercmd.h"

class EXPORT_GAME_SHARED CBasePlayerShared
{
public:
	CBasePlayerShared();

	virtual void setup_controller();
	virtual void player_move( CUserCmd *cmd, float frametime );

	int current_command_number() const;
	const CUserCmd *get_current_user_command() const;

	INLINE void set_view_angles( const LVector3 &angles )
	{
		_view_angles = angles;
	}

	INLINE LVector3 get_view_angles() const
	{
		return _view_angles;
	}

protected:
	LVector3 _view_angles;
	CUserCmd _current_cmd;
	PT( PhysicsCharacterController ) _controller;
};

INLINE int CBasePlayerShared::current_command_number() const
{
	return _current_cmd.commandnumber;
}

INLINE const CUserCmd *CBasePlayerShared::get_current_user_command() const
{
	return &_current_cmd;
}