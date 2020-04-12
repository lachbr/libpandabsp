#include "baseplayer.h"
#include "playercomponent.h"
#include "physics_character_controller.h"
#include "baseanimating_shared.h"
#include "masks.h"
#include "sv_bsploader.h"
#include "physicssystem.h"
#include "serverbase.h"
#include "scenecomponent_shared.h"
#include "in_buttons.h"
#include "usercmd.h"

class CCIPlayerComponent : public CPlayerComponent
{
	DECLARE_SERVERCLASS( CCIPlayerComponent, CPlayerComponent )

public:
	CCIPlayerComponent();

	virtual bool initialize();
	virtual void shutdown();

	virtual void player_run_command( CUserCmd *cmd, float frametime );

	PT( PhysicsCharacterController ) _controller;
	CSceneComponent *scene;
};

CCIPlayerComponent::CCIPlayerComponent() :
	CPlayerComponent(),
	_controller( nullptr ),
	scene( nullptr )
{
}

void CCIPlayerComponent::shutdown()
{
	if ( _controller )
	{
		_controller->remove_capsules();
		_controller = nullptr;
	}

	BaseClass::shutdown();
}

bool CCIPlayerComponent::initialize()
{
	if ( !BaseClass::initialize() )
	{
		return false;
	}

	PhysicsSystem *phys;
	sv->get_game_system_of_type( phys );
	_controller = new PhysicsCharacterController( svbsp->get_bsp_loader(), phys->get_physics_world(), sv->get_root(),
						      sv->get_root(), 4.0f, 2.0f, 0.3f, 1.0f,
						      phys->get_physics_world()->get_gravity()[2],
						      wall_mask, floor_mask, event_mask );
	_controller->set_max_slope( 75.0f, false );
	_controller->set_collide_mask( character_mask );
	_entity->get_component( scene );
	scene->np.reparent_to( _controller->get_movement_parent() );
	scene->np = _controller->get_movement_parent();

	return true;
}

void CCIPlayerComponent::player_run_command( CUserCmd *cmd, float frametime )
{
	BaseClass::player_run_command( cmd, frametime );

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

	scene->set_angles( LVector3f( _view_angles[0], scene->angles.get()[1], scene->angles.get()[2] ) );

	LVector3f vel( cmd->sidemove, cmd->forwardmove, 0.0f );
	LQuaternionf quat;
	quat.set_hpr( scene->angles.get() );
	vel = quat.xform( vel );

	_controller->set_linear_movement( vel );
	_controller->set_angular_movement( 0.0f );

	_controller->update( frametime );

	scene->origin = scene->np.get_pos();
	scene->angles = scene->np.get_hpr();
}

IMPLEMENT_SERVERCLASS_ST_OWNRECV( CCIPlayerComponent )
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

	PT( CCIPlayerComponent ) controls = new CCIPlayerComponent;
	add_component( controls );

	PT( CBaseAnimating ) anim = new CBaseAnimating;
	add_component( anim );
	animating = anim;
}