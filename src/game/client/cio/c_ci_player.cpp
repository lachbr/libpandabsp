#include "c_playercontrolled.h"
#include "physics_character_controller.h"
#include "masks.h"
#include "cl_bsploader.h"
#include "physicssystem.h"
#include "cl_rendersystem.h"
#include "clientbase.h"
#include "scenecomponent_shared.h"
#include "in_buttons.h"

class C_CIPlayerControlled : public C_PlayerControlled
{
	DECLARE_CLIENTCLASS( C_CIPlayerControlled, C_PlayerControlled )

public:
	C_CIPlayerControlled();

	virtual bool initialize();
	virtual void shutdown();

	virtual void player_run_command( CUserCmd *cmd, float frametime );

	PT( PhysicsCharacterController ) _controller;
};

C_CIPlayerControlled::C_CIPlayerControlled() :
	C_PlayerControlled(),
	_controller( nullptr )
{
}

void C_CIPlayerControlled::shutdown()
{
	if ( _controller )
	{
		_controller->remove_capsules();
		_controller = nullptr;
	}

	BaseClass::shutdown();
}

bool C_CIPlayerControlled::initialize()
{
	if ( !BaseClass::initialize() )
	{
		return false;
	}

	PhysicsSystem *phys;
	cl->get_game_system_of_type( phys );
	_controller = new PhysicsCharacterController( clbsp->get_bsp_loader(), phys->get_physics_world(), clrender->get_render(),
						      clrender->get_render(), 4.0f, 2.0f, 0.3f, 1.0f,
						      phys->get_physics_world()->get_gravity()[0],
						      wall_mask, floor_mask, event_mask );
	C_SceneComponent *scene;
	_entity->get_component( scene );
	scene->np.reparent_to( _controller->get_movement_parent() );
	scene->np = _controller->get_movement_parent();

	return true;
}

void C_CIPlayerControlled::player_run_command( CUserCmd *cmd, float frametime )
{
	nassertv( _controller != nullptr );

	if ( cmd->buttons & IN_JUMP )
	{
		std::cout << "Start jump!!!" << std::endl;
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
	_controller->set_angular_movement( 0.0f );

	_controller->update( frametime );
}

IMPLEMENT_CLIENTCLASS_RT_NOBASE( C_CIPlayerControlled, CCIPlayerControlled )
END_RECV_TABLE()