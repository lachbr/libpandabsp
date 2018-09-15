/**
 * \file rnCrowdAgent.h
 *
 * \date 2016-03-16
 * \author consultit
 */

#ifndef RNCROWDAGENT_H_
#define RNCROWDAGENT_H_

#include "config_module.h"

#include "rnNavMesh.h"
#include "rnNavMeshManager.h"
#include "rnTools.h"
#include "recastnavigation_includes.h"
#include "nodePath.h"

/**
 * This class represents a "crowd agent" of the RecastNavigation library.
 *
 * \see
 * 		- https://github.com/recastnavigation/recastnavigation.git
 * 		- http://digestingduck.blogspot.it
 * 		- https://groups.google.com/forum/?fromgroups#!forum/recastnavigation
 *
 * This PandaNode must be added to a RNNavMesh to be driven to its own target.\n
 * A model could be reparented to this RNCrowdAgent.\n
 * An RNCrowdAgent could be of type:
 * - **recast** (the default): its movement/orientation follows strictly the
 * path as updated by RecastNavigation library
 * - **kinematic**: its movement/orientation is corrected to stand on floor.\n
 * If enabled, this object can throw these events:
 * - on moving (default event name: NODENAME_CrowdAgent_Move)
 * - on being steady (default event name: NODENAME_CrowdAgent_Steady)
 * Events are thrown continuously at a frequency which is the minimum between
 * the fps and the frequency specified (which defaults to 30 times per seconds).
 * \n
 * The argument of each event is a reference to this component.\n
 *
 * \note A RNCrowdAgent will be reparented to the default reference node on
 * creation (see RNNavMeshManager).
 *
 * > **RNCrowdAgent text parameters**:
 * param | type | default | note
 * ------|------|---------|-----
 * | *thrown_events*				|single| - | specified as "event1@[event_name1]@[frequency1][:...[:eventN@[event_nameN]@[frequencyN]]]" with eventX = move,steady
 * | *add_to_navmesh*				|single| - | -
 * | *mov_type*						|single| *recast* | values: recast,kinematic
 * | *move_target*					|single| 0.0,0.0,0.0 | -
 * | *move_velocity*				|single| 0.0,0.0,0.0 | -
 * | *max_acceleration*			    |single| 8.0 | -
 * | *max_speed*					|single| 3.5 | -
 * | *collision_query_range*		|single| 12.0 | * RNNavMesh::agent_radius
 * | *path_optimization_range*		|single| 30.0 | * RNNavMesh::agent_radius
 * | *separation_weight* 			|single| 2.0 | -
 * | *update_flags*					|single| *0x1b* | -
 * | *obstacle_avoidance_type*		|single| *3* | values: 0,1,2,3
 *
 * \note parts inside [] are optional.\n
 */
class EXPCL_PANDARN RNCrowdAgent: public PandaNode
{
PUBLISHED:
	/**
	 * RNCrowdAgent movement type.
	 */
	enum RNCrowdAgentMovType
	{
		RECAST,
		RECAST_KINEMATIC,
		AgentMovType_NONE
	};

	/**
	 * RNCrowdAgent thrown events.
	 */
	enum RNEventThrown
	{
		MOVE_EVENT,
		STEADY_EVENT
	};

	/**
	 * Equivalent to Detour UpdateFlags enum.
	 */
	enum RNUpdateFlags
	{
#ifndef CPPPARSER
		ANTICIPATE_TURNS = DT_CROWD_ANTICIPATE_TURNS,
		OBSTACLE_AVOIDANCE = DT_CROWD_OBSTACLE_AVOIDANCE,
		SEPARATION = DT_CROWD_SEPARATION,
		OPTIMIZE_VIS = DT_CROWD_OPTIMIZE_VIS, // Use dtPathCorridor::optimizePathVisibility() to optimize the agent path.
		OPTIMIZE_TOPO = DT_CROWD_OPTIMIZE_TOPO, // Use dtPathCorridor::optimizePathTopology() to optimize the agent path.
#else
		ANTICIPATE_TURNS,OBSTACLE_AVOIDANCE,SEPARATION,
		OPTIMIZE_VIS,OPTIMIZE_TOPO,
#endif //CPPPARSER
	};

	/**
	 * Equivalent to Detour CrowdAgentState enum.
	 */
	enum RNCrowdAgentState
	{
#ifndef CPPPARSER
		STATE_INVALID = DT_CROWDAGENT_STATE_INVALID,	///< The agent is not in a valid state.
		STATE_WALKING = DT_CROWDAGENT_STATE_WALKING,	///< The agent is traversing a normal navigation mesh polygon.
		STATE_OFFMESH = DT_CROWDAGENT_STATE_OFFMESH,	///< The agent is traversing an off-mesh connection.
#else
		STATE_INVALID,STATE_WALKING,STATE_OFFMESH,
#endif //CPPPARSER
	};

	// To avoid interrogatedb warning.
#ifdef CPPPARSER
	virtual ~RNCrowdAgent();
#endif //CPPPARSER

	/**
	 * \name REFERENCE NODE
	 */
	///@{
	INLINE void set_reference_node_path(const NodePath& reference);
	///@}

	/**
	 * \name CONFIGURATION PARAMETERS
	 */
	///@{
	int set_params(const RNCrowdAgentParams& agentParams);
	INLINE RNCrowdAgentParams get_params() const;
	void set_mov_type(RNCrowdAgentMovType movType);
	INLINE RNCrowdAgentMovType get_mov_type() const;
	INLINE PT(RNNavMesh) get_nav_mesh() const;
	///@}

	/**
	 * \name MOTION STATUS AND PARAMETERS
	 */
	///@{
	int set_move_target(const LPoint3f& pos);
	INLINE LPoint3f get_move_target() const;
	int set_move_velocity(const LVector3f& vel);
	INLINE LVector3f get_move_velocity() const;
	LVector3f get_actual_velocity() const;
	RNCrowdAgentState get_traversing_state() const;
	///@}

	/**
	 * \name EVENTS' CONFIGURATION
	 */
	///@{
	INLINE void enable_throw_event(RNEventThrown event, ThrowEventData eventData);
	///@}

	/**
	 * \name OUTPUT
	 */
	///@{
	void output(ostream &out) const;
	///@}

#if defined(PYTHON_BUILD) || defined(CPPPARSER)
	/**
	 * \name PYTHON UPDATE CALLBACK
	 */
	///@{
	void set_update_callback(PyObject *value);
	///@}
#else
	/**
	 * \name C++ UPDATE CALLBACK
	 */
	///@{
	typedef void (*UPDATECALLBACKFUNC)(PT(RNCrowdAgent));
	void set_update_callback(UPDATECALLBACKFUNC value);
	///@}
#endif //PYTHON_BUILD

protected:
	friend void unref_delete<RNCrowdAgent>(RNCrowdAgent*);
	friend class RNNavMeshManager;
	friend class RNNavMesh;

	RNCrowdAgent(const string& name);
	virtual ~RNCrowdAgent();

private:
	///This NodePath.
	NodePath mThisNP;
	///The RNNavMesh this RNCrowdAgent is added to.
	PT(RNNavMesh) mNavMesh;
	///The reference node path.
	NodePath mReferenceNP;
	///The movement type.
	RNCrowdAgentMovType mMovType;
	///The RNCrowdAgent index.
	int mAgentIdx;
	///The associated dtCrowdAgent data.
	///@{
	RNCrowdAgentParams mAgentParams;
	LPoint3f mMoveTarget;
	LVector3f mMoveVelocity;
	///@}
	///Height correction for kinematic RNCrowdAgent(s).
	LVector3f mHeigthCorrection;

	inline void do_reset();
	void do_initialize();
	void do_finalize();

	void do_update_pos_dir(float dt, const LPoint3f& pos, const LVector3f& vel);

	/**
	 * Throwing RNCrowdAgent events.
	 */
	///@{
	ThrowEventData mMove, mSteady;
	///Helper.
	void do_enable_crowd_agent_event(RNEventThrown event, ThrowEventData eventData);
	void do_throw_event(ThrowEventData& eventData);
	///@}

#if defined(PYTHON_BUILD) || defined(CPPPARSER)
	/**
	 * \name Python callback.
	 */
	///@{
	PyObject *mSelf;
	PyObject *mUpdateCallback;
	PyObject *mUpdateArgList;
	///@}
#else
	/**
	 * \name C++ callback.
	 */
	///@{
	UPDATECALLBACKFUNC mUpdateCallback;
	///@}
#endif //PYTHON_BUILD

	// Explicitly disabled copy constructor and copy assignment operator.
	RNCrowdAgent(const RNCrowdAgent&);
	RNCrowdAgent& operator=(const RNCrowdAgent&);

public:
	/**
	 * \name TypedWritable API
	 */
	///@{
	static void register_with_read_factory();
	virtual void write_datagram(BamWriter *manager, Datagram &dg) override;
	virtual int complete_pointers(TypedWritable **plist, BamReader *manager) override;
	///@}

protected:
	static TypedWritable *make_from_bam(const FactoryParams &params);
	virtual void fillin(DatagramIterator &scan, BamReader *manager) override;

public:
	/**
	 * \name TypedObject API
	 */
	///@{
	static TypeHandle get_class_type()
	{
		return _type_handle;
	}
	static void init_type()
	{
		PandaNode::init_type();
		register_type(_type_handle, "RNCrowdAgent", PandaNode::get_class_type());
	}
	virtual TypeHandle get_type() const override
	{
		return get_class_type();
	}
	virtual TypeHandle force_init_type() override
	{
		init_type();
		return get_class_type();
	}
	///@}

private:
	static TypeHandle _type_handle;

};

INLINE ostream &operator << (ostream &out, const RNCrowdAgent & crowdAgent);

///inline
#include "rnCrowdAgent.I"

#endif /* RNCROWDAGENT_H_ */
