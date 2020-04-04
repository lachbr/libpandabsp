#include "baseplayer_shared.h"
//#include "basegame_shared.h"
#include "masks.h"

#include "in_buttons.h"

CBasePlayerShared::CBasePlayerShared() :
	_controller( nullptr )
{
}

void CBasePlayerShared::setup_controller()
{
	//_controller = new PhysicsCharacterController( g_game->_physics_world, g_game->_render,
	//					      g_game->_render, 4.0f, 2.0f, 0.3f, 1.0f,
	//					      g_game->_physics_world->get_gravity()[0],
	//					      wall_mask, floor_mask, event_mask );
}

void CBasePlayerShared::player_move( CUserCmd *cmd, float frametime )
{
	nassertv( _controller != nullptr );

	if ( cmd->buttons & IN_JUMP )
	{
		_controller->start_jump();
	}

	if ( cmd->buttons & IN_DUCK )
	{
		_controller->start_crouch();
	}
	else
	{
		_controller->stop_crouch();
	}

	LVector3f vel( cmd->sidemove, cmd->forwardmove, 0.0f );
	_controller->set_linear_movement( vel );
	_controller->set_angular_movement( cmd->mousedx );

	_controller->update( frametime );
}