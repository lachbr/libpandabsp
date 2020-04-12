#include "player_controls.h"
#include "usercmd.h"
#include "in_buttons.h"
#include "clientbase.h"
#include "cl_input.h"
#include "cl_rendersystem.h"

INLINE static float apply_friction( float goal, float last, float factor )
{
	return ( goal * factor + last * ( 1 - factor ) );
}

PlayerControls::PlayerControls()
{
	_slide_factor = 0.75f;
	_diagonal_factor = sqrt( 2.0f ) / 2.0f;

	_static_friction = 0.8f;
	_dynamic_friction = 0.3f;

	_goal_forward = 0.0f;
	_goal_side = 0.0f;
	_last_forward = 0.0f;
	_last_side = 0.0f;
}

float PlayerControls::get_walk_speed() const
{
	return 190 / 16.0f;
}

float PlayerControls::get_norm_speed() const
{
	return 320 / 16.0f;
}

float PlayerControls::get_run_speed() const
{
	return 416 / 16.0f;
}

float PlayerControls::get_duck_speed() const
{
	return 75 / 16.0f;
}

float PlayerControls::get_look_factor() const
{
	return cl_mousesensitivity.get_value();
}

void PlayerControls::do_controls( CUserCmd *cmd )
{
	float frametime = cl->get_frametime();

	// Determine goal speeds

	_goal_forward = 0.0f;
	_goal_side = 0.0f;

	// TODO: make this work with gamepads

	if ( cmd->buttons & IN_FORWARD )
	{
		if ( cmd->buttons & IN_DUCK )
		{
			_goal_forward += get_duck_speed();
		}
		else if ( cmd->buttons & IN_SPEED )
		{
			_goal_forward += get_run_speed();
		}
		else if ( cmd->buttons & IN_WALK )
		{
			_goal_forward += get_walk_speed();
		}
		else
		{
			_goal_forward += get_norm_speed();
		}
	}

	if ( cmd->buttons & IN_BACK )
	{
		if ( cmd->buttons & IN_DUCK )
		{
			_goal_forward -= get_duck_speed();
		}
		else if ( cmd->buttons & IN_SPEED )
		{
			_goal_forward -= get_run_speed();
		}
		else if ( cmd->buttons & IN_WALK )
		{
			_goal_forward -= get_walk_speed();
		}
		else
		{
			_goal_forward -= get_norm_speed();
		}
	}

	if ( cmd->buttons & IN_MOVERIGHT )
	{
		if ( cmd->buttons & IN_DUCK )
		{
			_goal_side += get_duck_speed();
		}
		else if ( cmd->buttons & IN_SPEED )
		{
			_goal_side += get_run_speed();
		}
		else if ( cmd->buttons & IN_WALK )
		{
			_goal_side += get_walk_speed();
		}
		else
		{
			_goal_side += get_norm_speed();
		}
	}

	if ( cmd->buttons & IN_MOVELEFT )
	{
		if ( cmd->buttons & IN_DUCK )
		{
			_goal_side -= get_duck_speed();
		}
		else if ( cmd->buttons & IN_SPEED )
		{
			_goal_side -= get_run_speed();
		}
		else if ( cmd->buttons & IN_WALK )
		{
			_goal_side -= get_walk_speed();
		}
		else
		{
			_goal_side -= get_norm_speed();
		}
	}

	// Apply smoothed out movement (friction)

	float sfriction = 1 - powf( 1 - _static_friction, frametime * 30.0f );
	float dfriction = 1 - powf( 1 - _dynamic_friction, frametime * 30.0f );

	float mod_forward = 0.0f;
	float mod_side = 0.0f;

	if ( fabsf( _goal_side ) < fabsf( _last_side ) )
	{
		mod_side = apply_friction( _goal_side, _last_side, dfriction );
	}
	else
	{
		mod_side = apply_friction( _goal_side, _last_side, sfriction );
	}

	if ( fabsf( _goal_forward ) < fabsf( _last_forward ) )
	{
		mod_forward = apply_friction( _goal_forward, _last_forward, dfriction );
	}
	else
	{
		mod_forward = apply_friction( _goal_forward, _last_forward, sfriction );
	}

	//std::cout << mod_forward << " | " << mod_side << std::endl;
	// This is the speed we will use
	cmd->forwardmove = mod_forward;
	cmd->sidemove = mod_side;
	_last_forward = mod_forward;
	_last_side = mod_side;

	LVector2f mouse_delta;
	clinput->get_mouse_delta_and_center( mouse_delta );
	mouse_delta *= get_look_factor();

	NodePath camera = clrender->get_camera();
	camera.set_h( camera.get_h() - mouse_delta.get_x() );
	camera.set_p( camera.get_p() - mouse_delta.get_y() );

	cmd->viewangles = camera.get_hpr();
}