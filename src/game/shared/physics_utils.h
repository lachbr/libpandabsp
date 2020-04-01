#pragma once

#include "config_game_shared.h"

#include <nodePath.h>
#include <vector_string.h>
#include <bulletAllHitsRayResult.h>

struct RayTestClosestNotMeResult_t
{
	bool result;
	BulletRayHit hit;
};

EXPORT_GAME_SHARED pvector<BulletRayHit> ray_test_all_stored( BulletWorld *world, const LPoint3 &from, const LPoint3 &to,
					   const BitMask32 &mask = BitMask32::all_on() );
EXPORT_GAME_SHARED void ray_test_closest_not_me( BulletWorld *world, RayTestClosestNotMeResult_t &result,
			      const NodePath &me, const LPoint3 &from, const LPoint3 &to,
			      const BitMask32 &mask = BitMask32::all_on() );

EXPORT_GAME_SHARED void create_and_attach_bullet_nodes( const NodePath &root_node );
EXPORT_GAME_SHARED void attach_bullet_nodes( BulletWorld *world, const NodePath &root_node );
EXPORT_GAME_SHARED void remove_bullet_nodes( const NodePath &root_node );
EXPORT_GAME_SHARED void detach_bullet_nodes( BulletWorld *world, const NodePath &root_node );

EXPORT_GAME_SHARED void detach_and_remove_bullet_nodes( const NodePath &root_node );

EXPORT_GAME_SHARED LVector3 get_throw_vector( BulletWorld *world, const LPoint3 &trace_origin, const LVector3 &trace_vector,
			   const LPoint3 &throw_origin, const NodePath &me );

EXPORT_GAME_SHARED void optimize_phys( const NodePath &root );
EXPORT_GAME_SHARED void make_bullet_coll_from_panda_coll( const NodePath &root_node, const vector_string &exclusions = vector_string() );