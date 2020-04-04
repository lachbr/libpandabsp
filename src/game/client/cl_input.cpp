#include "cl_input.h"
#include "usercmd.h"
#include "in_buttons.h"
#include <mouseWatcher.h>
#include "clientbase.h"
#include "cl_rendersystem.h"
#include "inputsystem.h"

CInput *clinput = nullptr;

IMPLEMENT_CLASS( CInput )

CInput::CInput() :
	InputSystem( cl ),
	_enabled( false )
{
	clinput = this;
}

float CInput::get_walk_speed() const
{
	return cl_walkspeed;
}

float CInput::get_norm_speed() const
{
	return cl_normspeed;
}

float CInput::get_run_speed() const
{
	return cl_runspeed;
}

float CInput::get_duck_speed() const
{
	return cl_duckspeed;
}

void CInput::setup_move( CUserCmd *cmd )
{
	if ( cmd->buttons & IN_FORWARD )
	{
		if ( cmd->buttons & IN_DUCK )
		{
			cmd->forwardmove += get_duck_speed();
		}
		else if ( cmd->buttons & IN_SPEED )
		{
			cmd->forwardmove += get_run_speed();
		}
		else if ( cmd->buttons & IN_WALK )
		{
			cmd->forwardmove += get_walk_speed();
		}
		else
		{
			cmd->forwardmove += get_norm_speed();
		}
	}

	if ( cmd->buttons & IN_BACK )
	{
		if ( cmd->buttons & IN_DUCK )
		{
			cmd->forwardmove -= get_duck_speed();
		}
		else if ( cmd->buttons & IN_SPEED )
		{
			cmd->forwardmove -= get_run_speed();
		}
		else if ( cmd->buttons & IN_WALK )
		{
			cmd->forwardmove -= get_walk_speed();
		}
		else
		{
			cmd->forwardmove -= get_norm_speed();
		}
	}

	if ( cmd->buttons & IN_MOVERIGHT )
	{
		if ( cmd->buttons & IN_DUCK )
		{
			cmd->sidemove += get_duck_speed();
		}
		else if ( cmd->buttons & IN_SPEED )
		{
			cmd->sidemove += get_run_speed();
		}
		else if ( cmd->buttons & IN_WALK )
		{
			cmd->sidemove += get_walk_speed();
		}
		else
		{
			cmd->sidemove += get_norm_speed();
		}
	}

	if ( cmd->buttons & IN_MOVELEFT )
	{
		if ( cmd->buttons & IN_DUCK )
		{
			cmd->sidemove -= get_duck_speed();
		}
		else if ( cmd->buttons & IN_SPEED )
		{
			cmd->sidemove -= get_run_speed();
		}
		else if ( cmd->buttons & IN_WALK )
		{
			cmd->sidemove -= get_walk_speed();
		}
		else
		{
			cmd->sidemove -= get_norm_speed();
		}
	}
}

void CInput::setup_mouse( CUserCmd *cmd )
{
	// If controls not enabled, don't do mouse movement.
	// Just send zeros.
	if ( !is_enabled() )
		return;

	MouseData md = clrender->get_graphics_window()->get_pointer( 0 );
	if ( md.get_in_window() )
	{
		LVector2 center( clrender->get_graphics_window()->get_x_size() / 2.0f,
				 clrender->get_graphics_window()->get_y_size() / 2.0f );

		cmd->mousedx = ( md.get_x() - center.get_x() ) * cl_mousesensitivity;
		cmd->mousedy = ( md.get_y() - center.get_y() ) * cl_mousesensitivity;

		clrender->get_graphics_window()->move_pointer( 0, (int)center[0], (int)center[1] );
	}
}

void CInput::setup_view( CUserCmd *cmd )
{
	ClientBase *game = ClientBase::ptr();
	cmd->viewangles = clrender->get_camera().get_hpr( clrender->get_render() );
}

void CInput::create_cmd( CUserCmd *cmd, int commandnumber, float input_sample_frametime, bool active )
{
	cmd->clear();

	cmd->commandnumber = commandnumber;
	cmd->tickcount = cl->get_tickcount();

	size_t count = get_num_mappings();
	for ( size_t i = 0; i < count; i++ )
	{
		const InputSystem::CKeyMapping &km = get_mapping( i );
		if ( km.is_down() )
			cmd->buttons |= km.button_type;
	}

	setup_move( cmd );
	setup_mouse( cmd );
	setup_view( cmd );
}

void CInput::set_enabled( bool enable )
{
	_enabled = enable;
}