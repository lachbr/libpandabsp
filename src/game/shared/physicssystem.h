#pragma once

#include "config_game_shared.h"
#include "igamesystem.h"

#include <bulletWorld.h>
#include <bulletConvexHullShape.h>
#include <collisionNode.h>

class EXPORT_GAME_SHARED PhysicsSystem : public IGameSystem
{
	DECLARE_CLASS( PhysicsSystem, IGameSystem )
public:
	PhysicsSystem();

	BulletWorld *get_physics_world() const;

	// Methods of IGameSystem
	virtual const char *get_name() const;
	virtual bool initialize();
	virtual void shutdown();
	virtual void update( double frametime );

public:
	// Physics helpers

	struct RayTestClosestNotMeResult_t
	{
		bool result;
		BulletRayHit hit;
	};

	pvector<BulletRayHit> ray_test_all_stored( const LPoint3 &from, const LPoint3 &to,
						   const BitMask32 &mask = BitMask32::all_on() );
	void ray_test_closest_not_me( RayTestClosestNotMeResult_t &result,
				      const NodePath &me, const LPoint3 &from, const LPoint3 &to,
				      const BitMask32 &mask = BitMask32::all_on() );

	void create_and_attach_bullet_nodes( const NodePath &root_node );
	void attach_bullet_nodes( const NodePath &root_node );
	void remove_bullet_nodes( const NodePath &root_node );
	void detach_bullet_nodes( const NodePath &root_node );

	void detach_and_remove_bullet_nodes( const NodePath &root_node );

	LVector3 get_throw_vector( const LPoint3 &trace_origin, const LVector3 &trace_vector,
				   const LPoint3 &throw_origin, const NodePath &me );

	void optimize_phys( const NodePath &root );
	void make_bullet_coll_from_panda_coll( const NodePath &root_node, const vector_string &exclusions = vector_string() );

	static PT( BulletConvexHullShape ) make_convex_hull_from_collision_solids( CollisionNode *cnode );
	static LVector3 extents_from_min_max( const LVector3 &min, const LVector3 &max );
	static LPoint3 center_from_min_max( const LVector3 &min, const LVector3 &max );

private:
	PT( BulletWorld ) _physics_world;
};

INLINE BulletWorld *PhysicsSystem::get_physics_world() const
{
	return _physics_world;
}

INLINE const char *PhysicsSystem::get_name() const
{
	return "PhysicsSystem";
}