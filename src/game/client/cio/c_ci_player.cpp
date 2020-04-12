#include "c_playercomponent.h"
#include "physics_character_controller.h"
#include "masks.h"
#include "cl_bsploader.h"
#include "physicssystem.h"
#include "cl_rendersystem.h"
#include "clientbase.h"
#include "scenecomponent_shared.h"
#include "in_buttons.h"
#include "player_controls.h"

class C_CIPlayerControlled : public C_PlayerComponent
{
	DECLARE_CLIENTCLASS( C_CIPlayerControlled, C_PlayerComponent )

public:
	C_CIPlayerControlled();

	virtual bool initialize();
	virtual void shutdown();
	
	virtual void create_cmd( CUserCmd *cmd );
	virtual void setup_view( CUserCmd *cmd );
	virtual void player_run_command( CUserCmd *cmd, float frametime );

	PT( PhysicsCharacterController ) _controller;
	C_SceneComponent *scene;
	PlayerControls controls;
};

C_CIPlayerControlled::C_CIPlayerControlled() :
	C_PlayerComponent(),
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
						      phys->get_physics_world()->get_gravity()[2],
						      wall_mask, floor_mask, event_mask );
	_controller->set_max_slope( 75.0f, false );
	_controller->set_collide_mask( character_mask );

	_entity->get_component( scene );
	scene->np.reparent_to( _controller->get_movement_parent() );
	scene->np = _controller->get_movement_parent();

	return true;
}

void C_CIPlayerControlled::create_cmd( CUserCmd *cmd )
{
	controls.do_controls( cmd );
}

void C_CIPlayerControlled::setup_view( CUserCmd *cmd )
{
	// View angles were already set up in PlayerControls...
	NodePath camera = clrender->get_camera();
	camera.set_pos( scene->origin + view_offset );
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
	LQuaternionf quat;
	quat.set_hpr( LVector3f( cmd->viewangles[0], scene->angles[1], scene->angles[2] ) );
	vel = quat.xform( vel );
	_controller->set_linear_movement( vel );
	_controller->set_angular_movement( 0.0f );

	_controller->update( frametime );
}

IMPLEMENT_CLIENTCLASS_RT( C_CIPlayerControlled, CCIPlayerComponent )
END_RECV_TABLE()