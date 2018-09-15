/**
 * \file rnNavMeshManager.I
 *
 * \date 2016-03-29
 * \author consultit
 */

#ifndef RNNAVMESHMANAGER_I_
#define RNNAVMESHMANAGER_I_

///RNNavMeshManager inline definitions

/**
 * Returns the reference NodePath.
 * The reference NodePath is the node with respect to which all RNNavMeshes,
 * RNCrowdAgents and obstacles (for OBSTACLE RNNavMesh type) are reparented
 * and with respect to which all placements are computed.
 */
INLINE NodePath RNNavMeshManager::get_reference_node_path() const
{
	return mReferenceNP;
}

/**
 * Sets the reference NodePath.
 * The reference NodePath is the node with respect to which all RNNavMeshes,
 * RNCrowdAgents and obstacles (for OBSTACLE RNNavMesh type) are reparented
 * and with respect to which all placements are computed.
 * This is usually called after restoring from a bam file.
 */
INLINE void RNNavMeshManager::set_reference_node_path(const NodePath& reference)
{
	mReferenceNP = reference;
}

/**
 * Returns the reference NodePath for debug drawing.
 * This node should be made sibling of the reference NodePath in the scene.
 */
INLINE NodePath RNNavMeshManager::get_reference_node_path_debug() const
{
	return mReferenceDebugNP;
}

/**
 * Returns the number of RNNavMeshes.
 */
INLINE int RNNavMeshManager::get_num_nav_meshes() const
{
	return (int) mNavMeshes.size();
}

/**
 * Returns the number of RNCrowdAgents.
 */
INLINE int RNNavMeshManager::get_num_crowd_agents() const
{
	return (int) mCrowdAgents.size();
}

/**
 * Get singleton pointer.
 */
INLINE RNNavMeshManager* RNNavMeshManager::get_global_ptr()
{
	if ( _global_ptr == nullptr )
	{
		_global_ptr = new RNNavMeshManager;
	}

	return _global_ptr;
}

INLINE void RNNavMeshManager::set_root_node_path( const NodePath &root )
{
	mRoot = root;
}

INLINE void RNNavMeshManager::set_collide_mask( const CollideMask &mask )
{
	mMask = mask;
}

/**
 * Get the collide mask.
 */
INLINE CollideMask RNNavMeshManager::get_collide_mask() const
{
	return mMask;
}

/**
 * Get the collision root.
 */
INLINE NodePath RNNavMeshManager::get_collision_root() const
{
	return mRoot;
}

/**
 * Get the collision traverser.
 */
INLINE CollisionTraverser* RNNavMeshManager::get_collision_traverser() const
{
	return mCTrav;
}

/**
 * Get the collision handler.
 */
INLINE CollisionHandlerQueue* RNNavMeshManager::get_collision_handler() const
{
	return mCollisionHandler;
}

/**
 * Get the collision ray.
 */
INLINE CollisionRay* RNNavMeshManager::get_collision_ray() const
{
	return mPickerRay;
}

#endif /* RNNAVMESHMANAGER_I_ */
