/**
 * \file rnNavMeshManager.h
 *
 * \date 2016-03-29
 * \author consultit
 */

#ifndef RNNAVMESHMANAGER_H_
#define RNNAVMESHMANAGER_H_

#include "rnTools.h"
#include "recastnavigation_includes.h"
#include "collisionTraverser.h"
#include "collisionHandlerQueue.h"
#include "collisionRay.h"

class RNNavMesh;
class RNCrowdAgent;

/**
 * This class manages RNNavMeshes and RNCrowdAgents creation/destruction.
 *
 * RNNavMeshManager is a singleton class.
 */
class EXPCL_PANDARN RNNavMeshManager: public TypedReferenceCount
{
PUBLISHED:
	RNNavMeshManager(const NodePath& root = NodePath(),
			const CollideMask& mask = GeomNode::get_default_collide_mask());
	virtual ~RNNavMeshManager();

	INLINE void set_root_node_path( const NodePath &root );
	INLINE void set_collide_mask( const CollideMask &mask );

	/**
	 * \name REFERENCE NODES
	 */
	///@{
	INLINE NodePath get_reference_node_path() const;
	INLINE void set_reference_node_path(const NodePath& reference);
	INLINE NodePath get_reference_node_path_debug() const;
	///@}

	/**
	 * \name RNNavMesh
	 */
	///@{
	NodePath create_nav_mesh();
	bool destroy_nav_mesh(NodePath navMeshNP);
	PT(RNNavMesh) get_nav_mesh(int index) const;
	INLINE int get_num_nav_meshes() const;
	MAKE_SEQ(get_nav_meshes, get_num_nav_meshes, get_nav_mesh);
	///@}

	/**
	 * \name RNCrowdAgent
	 */
	///@{
	NodePath create_crowd_agent(const string& name);
	bool destroy_crowd_agent(NodePath crowdAgentNP);
	RNCrowdAgent *get_crowd_agent(int index) const;
	INLINE int get_num_crowd_agents() const;
	MAKE_SEQ(get_crowd_agents, get_num_crowd_agents, get_crowd_agent);
	///@}

	/**
	 * The type of object for creation parameters.
	 */
	enum RNType
	{
		NAVMESH = 0,
		CROWDAGENT
	};

	/**
	 * \name TEXTUAL PARAMETERS
	 */
	///@{
	ValueList<string> get_parameter_name_list(RNType type) const;
	void set_parameter_values(RNType type, const string& paramName, const ValueList<string>& paramValues);
	ValueList<string> get_parameter_values(RNType type, const string& paramName) const;
	void set_parameter_value(RNType type, const string& paramName, const string& value);
	string get_parameter_value(RNType type, const string& paramName) const;
	void set_parameters_defaults(RNType type);
	///@}

	/**
	 * \name DEFAULT UPDATE
	 */
	///@{
	AsyncTask::DoneStatus update(GenericAsyncTask* task);
	void start_default_update();
	void stop_default_update();
	///@}

	/**
	 * \name SINGLETON
	 */
	///@{
	INLINE static RNNavMeshManager* get_global_ptr();
	///@}

	/**
	 * \name UTILITIES
	 */
	///@{
	float get_bounding_dimensions(NodePath modelNP, LVecBase3f& modelDims,
			LVector3f& modelDeltaCenter) const;
	Pair<bool,float> get_collision_height(const LPoint3f& origin,
			const NodePath& space = NodePath()) const;
	INLINE CollideMask get_collide_mask() const;
	INLINE NodePath get_collision_root() const;
	INLINE CollisionTraverser* get_collision_traverser() const;
	INLINE CollisionHandlerQueue* get_collision_handler() const;
	INLINE CollisionRay* get_collision_ray() const;
	///@}

	/**
	 * \name SERIALIZATION
	 */
	///@{
	bool write_to_bam_file(const string& fileName);
	bool read_from_bam_file(const string& fileName);
	///@}

	/**
	 * Equivalent to duDebugDrawPrimitives.
	 */
	enum RNDebugDrawPrimitives
	{
#ifndef CPPPARSER
		POINTS = DU_DRAW_POINTS,
		LINES = DU_DRAW_LINES,
		TRIS = DU_DRAW_TRIS,
		QUADS = DU_DRAW_QUADS,
#else
		POINTS,LINES,TRIS,QUADS
#endif //CPPPARSER
	};

	/**
	 * \name LOW LEVEL DEBUG DRAWING
	 */
	///@{
	void debug_draw_primitive(RNDebugDrawPrimitives primitive,
			const ValueList<LPoint3f>& points, const LVecBase4f color = LVecBase4f::zero(), float size =
					1.0f);
	void debug_draw_reset();
	///@}

private:
	///The reference node path.
	NodePath mReferenceNP;

	///List of RNNavMeshes handled by this template.
	typedef pvector<PT(RNNavMesh)> NavMeshList;
	NavMeshList mNavMeshes;
	///RNNavMeshes' parameter table.
	ParameterTable mNavMeshesParameterTable;

	///List of RNCrowdAgents handled by this template.
	typedef pvector<RNCrowdAgent *> CrowdAgentList;
	CrowdAgentList mCrowdAgents;
	///RNCrowdAgents' parameter table.
	ParameterTable mCrowdAgentsParameterTable;

	///@{
	///A task data for step simulation update.
	PT(TaskInterface<RNNavMeshManager>::TaskData) mUpdateData;
	PT(AsyncTask) mUpdateTask;
	///@}

	///Utilities.
	NodePath mRoot;
	CollideMask mMask; //a.k.a. BitMask32
	CollisionTraverser* mCTrav;
	CollisionHandlerQueue* mCollisionHandler;
	CollisionRay* mPickerRay;

	///The reference node path for debug drawing.
	NodePath mReferenceDebugNP;
#ifdef RN_DEBUG
	class DebugDrawPrimitives: public rnsup::DebugDrawPanda3d
	{
	public:
		DebugDrawPrimitives(NodePath render): rnsup::DebugDrawPanda3d(render)
		{
		}
		void vertex(const LVector3f& vertex, const LVector4f& color)
		{
			doVertex(vertex, color);
		}
	};
	/// DebugDrawers.
	DebugDrawPrimitives* mDD;
#endif //RN_DEBUG

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
		TypedReferenceCount::init_type();
		register_type(_type_handle, "RNNavMeshManager",
				TypedReferenceCount::get_class_type());
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
	static RNNavMeshManager *_global_ptr;

};

///inline
#include "rnNavMeshManager.I"

#endif /* RNNAVMESHMANAGER_H_ */
