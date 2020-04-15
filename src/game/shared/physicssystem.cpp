#include "physicssystem.h"
#include "masks.h"

#include <bulletTriangleMesh.h>
#include <bulletTriangleMeshShape.h>
#include <collisionPolygon.h>

IMPLEMENT_CLASS( PhysicsSystem )

static constexpr float meters_to_feet = 3.2808399f;
static constexpr float panda_phys_gravity = -9.8f * meters_to_feet;

static ConfigVariableInt phys_substeps( "phys_substeps", 1 );

PhysicsSystem::PhysicsSystem()
{
	_physics_world = nullptr;
}

bool PhysicsSystem::initialize()
{
	_physics_world = new BulletWorld;
	_physics_world->set_gravity( 0.0f, 0.0f, panda_phys_gravity );

	return true;
}

void PhysicsSystem::shutdown()
{

}

void PhysicsSystem::update( double frametime )
{
	if ( !_physics_world )
	{
		return;
	}

	if ( frametime <= 0.016 )
	{
		_physics_world->do_physics( frametime, phys_substeps, 0.016f );
	}
	else if ( frametime <= 0.033 )
	{
		_physics_world->do_physics( frametime, phys_substeps, 0.033f );
	}
	else
	{
		_physics_world->do_physics( frametime, 0 );
	}
}

pvector<BulletRayHit> PhysicsSystem::ray_test_all_stored( const LPoint3 &from, const LPoint3 &to,
					   const BitMask32 &mask )
{
	BulletAllHitsRayResult result = _physics_world->ray_test_all( from, to, mask );
	pvector<BulletRayHit> sorted_hits;

	if ( result.get_num_hits() > 0 )
	{
		for ( int i = 0; i < result.get_num_hits(); i++ )
		{
			sorted_hits.push_back( result.get_hit( i ) );
		}
		std::sort( sorted_hits.begin(), sorted_hits.end(), []( const BulletRayHit &a, const BulletRayHit &b )
		{
			return ( a.get_hit_fraction() < b.get_hit_fraction() );
		} );
	}

	return sorted_hits;
}

void PhysicsSystem::ray_test_closest_not_me( RayTestClosestNotMeResult_t &result, const NodePath &me,
			      const LPoint3 &from, const LPoint3 &to,
			      const BitMask32 &mask )
{
	result.result = false;

	if ( me.is_empty() )
		return;

	pvector<BulletRayHit> hits = ray_test_all_stored( from, to, mask );
	for ( BulletRayHit brh : hits )
	{
		NodePath hitnp( brh.get_node() );
		if ( !me.is_ancestor_of( hitnp ) && me != hitnp )
		{
			result.hit = brh;
			result.result = true;
			return;
		}
	}
}

void PhysicsSystem::optimize_phys( const NodePath &root )
{
	int colls = 0;
	NodePathCollection npc;

	NodePathCollection children = root.get_children();
	for ( int i = 0; i < children.get_num_paths(); i++ )
	{
		NodePath child = children[i];
		if ( child.node()->is_of_type( CollisionNode::get_class_type() ) )
		{
			colls++;
			npc.add_path( child );
		}
	}

	if ( colls > 1 )
	{
		NodePath collide = root.attach_new_node( "__collide__" );
		npc.wrt_reparent_to( collide );
		collide.clear_model_nodes();
		collide.flatten_strong();

		// Move the possibly combined CollisionNodes back to the root.
		collide.get_children().wrt_reparent_to( root );
		collide.remove_node();
	}

	children = root.get_children();
	for ( int i = 0; i < children.get_num_paths(); i++ )
	{
		NodePath child = children[i];
		if ( child.get_name() != "__collide__" )
			optimize_phys( child );
	}
}

void PhysicsSystem::make_bullet_coll_from_panda_coll( const NodePath &root_node, const vector_string &exclusions )
{
	// First combine any redundant CollisionNodes.
	optimize_phys( root_node );

	NodePathCollection npc = root_node.find_all_matches( "**" );
	for ( int i = 0; i < npc.get_num_paths(); i++ )
	{
		NodePath pcollnp = npc[i];
		if ( std::find( exclusions.begin(), exclusions.end(), pcollnp.get_name() ) != exclusions.end() )
			continue;
		if ( pcollnp.node()->get_type() != CollisionNode::get_class_type() )
			continue;

		CollisionNode *cnode = DCAST( CollisionNode, pcollnp.node() );

		if ( cnode->get_num_solids() == 0 )
			continue;

		PT( BulletBodyNode ) bnode;
		BitMask32 mask = cnode->get_into_collide_mask();
		bool is_ghost = ( !cnode->get_solid( 0 )->is_tangible() );
		if ( is_ghost )
		{
			bnode = new BulletGhostNode( cnode->get_name().c_str() );
			mask = event_mask;
		}
		else
		{
			bnode = new BulletRigidBodyNode( cnode->get_name().c_str() );
		}
		bnode->add_shapes_from_collision_solids( cnode );
		for ( int i = 0; i < bnode->get_num_shapes(); i++ )
		{
			BulletShape *shape = bnode->get_shape( i );
			if ( shape->is_of_type( BulletTriangleMeshShape::get_class_type() ) )
				shape->set_margin( 0.1f );
		}
		bnode->set_kinematic( true );
		NodePath bnp( bnode );
		bnp.reparent_to( pcollnp.get_parent() );
		bnp.set_transform( pcollnp.get_transform() );
		bnp.set_collide_mask( mask );
		// Now that we're using Bullet collisions, we don't need the panda collisions.
		pcollnp.remove_node();
	}
}

void PhysicsSystem::create_and_attach_bullet_nodes( const NodePath &root_node )
{
	make_bullet_coll_from_panda_coll( root_node );
	attach_bullet_nodes( root_node );
}

void PhysicsSystem::attach_bullet_nodes( const NodePath &root_node )
{
	if ( root_node.is_empty() )
		return;

	NodePathCollection npc;
	int i;

	npc = root_node.find_all_matches( "**/+BulletRigidBodyNode" );
	for ( i = 0; i < npc.get_num_paths(); i++ )
	{
		_physics_world->attach( npc[i].node() );
	}
	npc = root_node.find_all_matches( "**/+BulletGhostNode" );
	for ( i = 0; i < npc.get_num_paths(); i++ )
	{
		_physics_world->attach( npc[i].node() );
	}
}

void PhysicsSystem::detach_bullet_nodes( const NodePath &root_node )
{
	if ( root_node.is_empty() )
		return;

	NodePathCollection npc;
	int i;

	npc = root_node.find_all_matches( "**/+BulletRigidBodyNode" );
	for ( i = 0; i < npc.get_num_paths(); i++ )
	{
		_physics_world->remove( npc[i].node() );
	}
	npc = root_node.find_all_matches( "**/+BulletGhostNode" );
	for ( i = 0; i < npc.get_num_paths(); i++ )
	{
		_physics_world->remove( npc[i].node() );
	}
}

void PhysicsSystem::remove_bullet_nodes( const NodePath &root_node )
{
	if ( root_node.is_empty() )
		return;

	NodePathCollection npc;
	int i;

	npc = root_node.find_all_matches( "**/+BulletRigidBodyNode" );
	for ( i = 0; i < npc.get_num_paths(); i++ )
	{
		npc[i].remove_node();
	}
	npc = root_node.find_all_matches( "**/+BulletGhostNode" );
	for ( i = 0; i < npc.get_num_paths(); i++ )
	{
		npc[i].remove_node();
	}
}

void PhysicsSystem::detach_and_remove_bullet_nodes( const NodePath &root_node )
{
	detach_bullet_nodes( root_node );
	remove_bullet_nodes( root_node );
}

LVector3 PhysicsSystem::get_throw_vector( const LPoint3 &trace_origin, const LVector3 &trace_vector,
			   const LPoint3 &throw_origin, const NodePath &me )
{
	LPoint3 trace_end = trace_origin + ( trace_vector * 10000 );
	RayTestClosestNotMeResult_t result;
	ray_test_closest_not_me( result, me,
				 trace_origin,
				 trace_end,
				 WORLD_MASK );
	LPoint3 hit_pos;
	if ( result.result )
		hit_pos = trace_end;
	else
		hit_pos = result.hit.get_hit_pos();

	return ( hit_pos - throw_origin ).normalized();
}

PT( BulletConvexHullShape ) PhysicsSystem::make_convex_hull_from_collision_solids( CollisionNode *cnode )
{
	PT( BulletConvexHullShape ) shape = new BulletConvexHullShape;
	int num_solids = cnode->get_num_solids();
	for ( int i = 0; i < num_solids; i++ )
	{
		const CollisionSolid *solid = cnode->get_solid( i );
		if ( solid->is_of_type( CollisionPolygon::get_class_type() ) )
		{
			const CollisionPolygon *poly = DCAST( CollisionPolygon, solid );
			int num_points = poly->get_num_points();
			for ( int j = 0; j < num_points; j++ )
			{
				LPoint3 pt = poly->get_point( j );
				shape->add_point( pt );
			}
		}
	}
	return shape;
}

LVector3 PhysicsSystem::extents_from_min_max( const LVector3 &min, const LVector3 &max )
{
	return LVector3( ( max[0] - min[0] ) / 2.0f,
			 ( max[1] - min[1] ) / 2.0f,
			 ( max[2] - min[2] ) / 2.0f );
}

LPoint3 PhysicsSystem::center_from_min_max( const LVector3 &min, const LVector3 &max )
{
	return ( min + max ) / 2.0f;
}