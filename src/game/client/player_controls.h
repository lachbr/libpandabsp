#pragma once

#include "config_clientdll.h"
#include "referenceCount.h"

class CUserCmd;

class EXPORT_CLIENT_DLL PlayerControls : public ReferenceCount
{
public:
	enum MoveType
	{
		MOVETYPE_WALK,
		MOVETYPE_FLY,
		MOVETYPE_SWIM,
	};

	virtual float get_walk_speed() const;
	virtual float get_norm_speed() const;
	virtual float get_run_speed() const;
	virtual float get_duck_speed() const;
	virtual float get_look_factor() const;

	PlayerControls();

	void set_move_type( MoveType type );

	virtual void do_controls( CUserCmd *cmd );

protected:
	float _goal_forward;
	float _goal_side;

	float _last_forward;
	float _last_side;

	float _static_friction;
	float _dynamic_friction;

	float _slide_factor;
	float _diagonal_factor;
	
	MoveType _move_type;
};

INLINE void PlayerControls::set_move_type( MoveType type )
{
	_move_type = type;
}