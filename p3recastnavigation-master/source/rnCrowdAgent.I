/**
 * \file rnCrowdAgent.I
 *
 * \date 2016-03-19
 * \author consultit
 */

#ifndef RNCROWDAGENT_I_
#define RNCROWDAGENT_I_

///RNCrowdAgent inline definitions

/**
 * Sets the reference NodePath of this RNCrowdAgent.
 */
INLINE void RNCrowdAgent::set_reference_node_path(const NodePath& reference)
{
	mReferenceNP = reference;
}

/**
 * Returns RNCrowdAgent's parameters.
 */
INLINE RNCrowdAgentParams RNCrowdAgent::get_params() const
{
	return mAgentParams;
}

/**
 * Returns RNCrowdAgent's move target.
 * Should be called after addition to a RNNavMesh.
 * Returns LPoint3f::zero() on error.
 */
INLINE LPoint3f RNCrowdAgent::get_move_target() const
{
	// continue if crowdAgent belongs to a mesh
	CONTINUE_IF_ELSE_R(mNavMesh, LPoint3f::zero())

	return mMoveTarget;
}

/**
 * Returns RNCrowdAgent's move velocity.
 * Should be called after addition to a RNNavMesh.
 * Returns LVector3f::zero() on error.
 */
INLINE LVector3f RNCrowdAgent::get_move_velocity() const
{
	// continue if crowdAgent belongs to a mesh
	CONTINUE_IF_ELSE_R(mNavMesh, LVector3f::zero())

	return mMoveVelocity;
}

/**
 * Returns RNCrowdAgent's movement type (eithr recast or kinematic).
 */
INLINE RNCrowdAgent::RNCrowdAgentMovType RNCrowdAgent::get_mov_type() const
{
	return mMovType;
}

/**
 * Returns the RNNavMesh this RNCrowdAgent is added to.
 */
INLINE PT(RNNavMesh) RNCrowdAgent::get_nav_mesh() const
{
	return mNavMesh;
}

/**
 * Enables/disables the RNCrowdAgent's events to be thrown.
 * \note Internal use only.
 */
INLINE void RNCrowdAgent::enable_throw_event(RNEventThrown event,
		ThrowEventData eventData)
{
	do_enable_crowd_agent_event(event, eventData);
}

/**
 * Resets RNCrowdAgent.
 * \note Internal use only.
 */
inline void RNCrowdAgent::do_reset()
{
	//
	mMovType = RECAST;
	mAgentIdx = -1;
	mAgentParams = RNCrowdAgentParams();
	mMoveTarget = LPoint3f::zero();
	mMoveVelocity = LVector3f::zero();
	mHeigthCorrection = LVector3f::zero();
	mMove = mSteady = ThrowEventData();
	mReferenceNP.clear();
#ifdef PYTHON_BUILD
	mSelf = NULL;
	mUpdateCallback = NULL;
	mUpdateArgList = NULL;
#else
	mUpdateCallback = NULL;
#endif //PYTHON_BUILD
}

INLINE ostream &operator <<(ostream &out, const RNCrowdAgent& crowdAgent)
{
	crowdAgent.output(out);
	return out;
}

#endif /* RNCROWDAGENT_I_ */
