#include "player_controls.h"
#include "usercmd.h"
#include "in_buttons.h"
#include "clientbase.h"
#include "cl_input.h"
#include "cl_rendersystem.h"

#define DEADZONE 0.1f
#define CONTROLLER_LOOK_FACTOR 100.0f

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

	bool speed = clinput->get_button_value( IN_SPEED );
	bool walk = clinput->get_button_value( IN_WALK );
	bool duck = clinput->get_button_value( IN_DUCK );

	float forward_factor = 0.0f;
	forward_factor += clinput->get_button_value( IN_FORWARD ) ? 1.0f : 0.0f;
	forward_factor -= clinput->get_button_value( IN_BACK ) ? 1.0f : 0.0f;

	float axis_forward = clinput->get_axis_value( AXIS_Y );
	if ( fabsf( axis_forward ) <= DEADZONE )
	{
		axis_forward = 0.0f;
	}
	forward_factor += axis_forward;
	//std::cout << "Y Axis: " << clinput->get_axis_value( AXIS_Y ) << std::endl;

	float side_factor = 0.0f;
	side_factor += clinput->get_button_value( IN_MOVERIGHT ) ? 1.0f : 0.0f;
	side_factor -= clinput->get_button_value( IN_MOVELEFT ) ? 1.0f : 0.0f;
	float axis_side = clinput->get_axis_value( AXIS_X );
	if ( fabsf( axis_side ) <= DEADZONE )
	{
		axis_side = 0.0f;
	}
	side_factor += axis_side;
	//std::cout << "X Axis: " << clinput->get_axis_value( AXIS_X ) << std::endl;

	if ( speed )
	{
		_goal_forward = get_run_speed();
		_goal_side = get_run_speed();
	}
	else if ( walk )
	{
		_goal_forward = get_walk_speed();
		_goal_side = get_walk_speed();
	}
	else if ( duck )
	{
		_goal_forward = get_duck_speed();
		_goal_side = get_duck_speed();
	}
	else
	{
		_goal_forward = get_norm_speed();
		_goal_side = get_norm_speed();
	}

	_goal_forward *= forward_factor;
	_goal_side *= side_factor;

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
	//clinput->get_mouse_delta_and_center( mouse_delta );
//	mouse_delta *= get_look_factor();

	float lookx = 0.0f;//mouse_delta[0];
	float looky = 0.0f;//mouse_delta[1];

	float axis_lookx = clinput->get_axis_value( AXIS_LOOK_X );
	if ( fabsf( axis_lookx ) <= DEADZONE )
	{
		axis_lookx = 0.0f;
	}
	lookx += (axis_lookx * CONTROLLER_LOOK_FACTOR) * frametime;
	
	float axis_looky = clinput->get_axis_value( AXIS_LOOK_Y );
	if ( fabsf( axis_looky ) <= DEADZONE )
	{
		axis_looky = 0.0f;
	}
	looky -= (axis_looky * CONTROLLER_LOOK_FACTOR) * frametime;

	//std::cout << "Look X Axis: " << clinput->get_axis_value( AXIS_LOOK_X ) << std::endl;
	//std::cout << "Look Y Axis: " << clinput->get_axis_value( AXIS_LOOK_Y ) << std::endl;

	NodePath camera = clrender->get_camera();
	camera.set_h( camera.get_h() - lookx );
	camera.set_p( camera.get_p() - looky );

	cmd->viewangles = camera.get_hpr();
}