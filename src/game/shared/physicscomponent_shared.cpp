#include "physicscomponent_shared.h"

#include "scenecomponent_shared.h"

#include "bulletBoxShape.h"

#ifdef CLIENT_DLL
#include "clientbase.h"
#define host cl
#else
#include "serverbase.h"
#define host sv
#endif

bool PhysicsComponent::initialize()
{
	if ( !BaseClass::initialize() )
	{
		return false;
	}

	if ( !_entity->get_component( scene ) )
	{
		return false;
	}

	bodynode = nullptr;
	physics_setup = false;
	solid = SOLID_NONE;
	phys_mask = BitMask32::all_on();
	mass = 0.0f;
	kinematic = false;

	return true;
}

PT( CIBulletRigidBodyNode ) PhysicsComponent::get_phys_body()
{
	PT( CIBulletRigidBodyNode ) bnode = new CIBulletRigidBodyNode( "entity-phys" );
	bnode->set_mass( mass );

	bnode->set_ccd_motion_threshold( 1e-7 );
	bnode->set_ccd_swept_sphere_radius( 0.5f );

	switch ( solid )
	{
	case SOLID_MESH:
		{
			CollisionNode *cnode = DCAST( CollisionNode, scene->np.find( "**/+CollisionNode" ).node() );
			PT( BulletConvexHullShape ) convex_hull = PhysicsSystem::
				make_convex_hull_from_collision_solids( cnode );
			bnode->add_shape( convex_hull );
			break;
		}
	case SOLID_BBOX:
		{
			LPoint3 mins, maxs;
			scene->np.calc_tight_bounds( mins, maxs );
			LVector3f extents = PhysicsSystem::extents_from_min_max( mins, maxs );
			CPT( TransformState ) ts_center = TransformState::make_pos(
				PhysicsSystem::center_from_min_max( mins, maxs ) );
			PT( BulletBoxShape ) shape = new BulletBoxShape( extents );
			bnode->add_shape( shape, ts_center );
			break;
		}
	case SOLID_SPHERE:
		{
			break;
		}
	}

	return bnode;
}

void PhysicsComponent::spawn()
{
	if ( solid != SOLID_NONE )
	{
		bool underneath_self = !kinematic;
		bodynode = get_phys_body();
		bodynode->set_user_data( _entity );
		bodynode->set_kinematic( underneath_self );

		NodePath parent = scene->np.get_parent();
		bodynp = parent.attach_new_node( bodynode );
		bodynp.set_collide_mask( phys_mask );
		if ( !underneath_self )
		{
			scene->np.reparent_to( bodynp );
			scene->np = bodynp;
		}
		else
		{
			bodynp.reparent_to( scene->np );
		}

		PhysicsSystem *phys;
		host->get_game_system( phys );
		phys->get_physics_world()->attach( bodynode );

		physics_setup = true;
	}
}

void PhysicsComponent::update( double frametime )
{
#if !defined( CLIENT_DLL )
	if ( physics_setup && kinematic )
	{
		scene->origin = bodynp.get_pos();
		scene->angles = bodynp.get_hpr();
	}
#endif
}

void PhysicsComponent::shutdown()
{
	if ( physics_setup )
	{
		PhysicsSystem *phys;
		host->get_game_system( phys );

		phys->get_physics_world()->remove( bodynode );
		bodynp.remove_node();
		bodynode = nullptr;
		physics_setup = false;
	}
}

#if !defined( CLIENT_DLL )

IMPLEMENT_SERVERCLASS_ST_NOBASE( CPhysicsComponent )
	SendPropInt( PROPINFO( phys_mask ) ),
	SendPropInt( PROPINFO( solid ) ),
END_SEND_TABLE()

#else

IMPLEMENT_CLIENTCLASS_RT_NOBASE( C_PhysicsComponent, CPhysicsComponent )
	RecvPropInt( PROPINFO( phys_mask ) ),
	RecvPropInt( PROPINFO( solid ) ),
END_RECV_TABLE()

#endif
