#include "cl_input.h"
#include "usercmd.h"
#include "globalvars_client.h"
#include "c_basegame.h"
#include "in_buttons.h"
#include <mouseWatcher.h>

CInput::CInput() :
	_enabled( false )
{
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

	ClientBase *game = ClientBase::ptr();
	MouseWatcher *mw = DCAST( MouseWatcher, game->_mouse_watcher[0].node() );
	if ( mw->has_mouse() )
	{
		MouseData md = game->get_graphics_window()->get_pointer( 0 );
		LVector2 center( game->get_graphics_window()->get_x_size() / 2.0f,
				 game->get_graphics_window()->get_y_size() / 2.0f );

		cmd->mousedx = ( md.get_x() - center.get_x() ) * cl_mousesensitivity;
		cmd->mousedy = ( md.get_y() - center.get_y() ) * cl_mousesensitivity;

		game->get_graphics_window()->move_pointer( 0, (int)center[0], (int)center[1] );
	}
}

void CInput::setup_view( CUserCmd *cmd )
{
	ClientBase *game = ClientBase::ptr();
	cmd->viewangles = game->_camera.get_hpr( game->_render );
}

void CInput::create_cmd( CUserCmd *cmd, int commandnumber, float input_sample_frametime, bool active )
{
	cmd->clear();

	cmd->commandnumber = commandnumber;
	cmd->tickcount = g_globals->tickcount;

	CKeyMappings *map = g_globals->game->get_key_mappings();
	for ( size_t i = 0; i < map->mappings.size(); i++ )
	{
		const CKeyMapping &km = map->mappings.get_data( i );
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