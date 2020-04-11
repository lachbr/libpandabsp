#include "baseplayer.h"
#include "playercontrolled.h"
#include "physics_character_controller.h"
#include "baseanimating_shared.h"
#include "masks.h"
#include "sv_bsploader.h"
#include "physicssystem.h"
#include "serverbase.h"
#include "scenecomponent_shared.h"
#include "in_buttons.h"
#include "usercmd.h"

class CCIPlayerControlled : public CPlayerControlled
{
	DECLARE_SERVERCLASS( CCIPlayerControlled, CPlayerControlled )

public:
	CCIPlayerControlled();

	virtual bool initialize();
	virtual void shutdown();

	virtual void player_run_command( CUserCmd *cmd, float frametime );

	PT( PhysicsCharacterController ) _controller;
	CSceneComponent *scene;
};

CCIPlayerControlled::CCIPlayerControlled() :
	CPlayerControlled(),
	_controller( nullptr ),
	scene( nullptr )
{
}

void CCIPlayerControlled::shutdown()
{
	if ( _controller )
	{
		_controller->remove_capsules();
		_controller = nullptr;
	}

	BaseClass::shutdown();
}

bool CCIPlayerControlled::initialize()
{
	if ( !BaseClass::initialize() )
	{
		return false;
	}

	PhysicsSystem *phys;
	sv->get_game_system_of_type( phys );
	_controller = new PhysicsCharacterController( svbsp->get_bsp_loader(), phys->get_physics_world(), sv->get_root(),
						      sv->get_root(), 4.0f, 2.0f, 0.3f, 1.0f,
						      phys->get_physics_world()->get_gravity()[0],
						      wall_mask, floor_mask, event_mask );
	_entity->get_component( scene );
	scene->np.reparent_to( _controller->get_movement_parent() );
	scene->np = _controller->get_movement_parent();

	return true;
}

void CCIPlayerControlled::player_run_command( CUserCmd *cmd, float frametime )
{
	nassertv( _controller != nullptr );

	if ( cmd->buttons & IN_JUMP )
	{
		//std::cout << "Start jump!!!" << std::endl;
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

	scene->origin = scene->np.get_pos();
	scene->angles = scene->np.get_hpr();
}

IMPLEMENT_SERVERCLASS_ST_NOBASE_OWNRECV( CCIPlayerControlled )
END_SEND_TABLE()

//-------------------------------------------------------------------------

class CCIPlayer : public CBasePlayer
{
	DECLARE_ENTITY( CCIPlayer, CBasePlayer );

public:
	virtual void add_components();
	virtual void spawn();

	CBaseAnimating *animating;
};

IMPLEMENT_ENTITY( CCIPlayer, ci_player )

void CCIPlayer::spawn()
{
	BaseClass::spawn();

	animating->set_model( "phase_14/models/misc/toon_ref.bam" );
}

void CCIPlayer::add_components()
{
	BaseClass::add_components();

	PT( CBaseAnimating ) anim = new CBaseAnimating;
	add_component( anim );
	animating = anim;

	PT( CCIPlayerControlled ) controls = new CCIPlayerControlled;
	add_component( controls );
}