#pragma once

#include "config_game_shared.h"
#include "shareddefs.h"
#include "physicssystem.h"

#ifdef CLIENT_DLL
#define PhysicsComponent C_PhysicsComponent
#define SceneComponent C_SceneComponent
#else
#define PhysicsComponent CPhysicsComponent
#define SceneComponent CSceneComponent
#endif

class SceneComponent;

class EXPORT_GAME_SHARED PhysicsComponent : public CBaseComponent
{
	DECLARE_COMPONENT( PhysicsComponent, CBaseComponent )
public:

	virtual bool initialize();
	virtual void spawn();
	virtual void shutdown();
	virtual void update( double frametime );

	enum SolidType
	{
		SOLID_BBOX,
		SOLID_MESH,
		SOLID_SPHERE,
		SOLID_NONE,
	};

	SceneComponent *scene;

	PT( CIBulletRigidBodyNode ) bodynode;
	NodePath bodynp;
	bool physics_setup;

	float mass;
	bool kinematic;

#if !defined( CLIENT_DLL )
	
	NetworkVar( BitMask32, phys_mask );
	NetworkVar( SolidType, solid );
#else
	BitMask32 phys_mask;
	SolidType solid;
#endif

private:
	PT( CIBulletRigidBodyNode ) get_phys_body();
};