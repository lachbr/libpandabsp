/**
 * \file rnNavMesh.h
 *
 * \date 2016-03-16
 * \author consultit
 */

#ifndef RNNAVMESH_H_
#define RNNAVMESH_H_

#include "rnTools.h"
#include "recastnavigation_includes.h"
#include "nodePath.h"

#ifndef CPPPARSER
#include "support/CrowdTool.h"
#include "support/NavMeshType_Tile.h"
#include "support/NavMeshType_Obstacle.h"
#include "support/NavMeshTesterTool.h"
#include "library/DetourTileCache.h"
#endif //CPPPARSER

class RNCrowdAgent;

/**
 * This class represents a "navigation mesh" of the RecastNavigation library.
 *
 * \see
 * 		- https://github.com/recastnavigation/recastnavigation.git
 * 		- http://digestingduck.blogspot.it
 * 		- https://groups.google.com/forum/?fromgroups#!forum/recastnavigation
 *
 * This PandaNode will create a "navigation mesh" for its "owner object", which
 * is a model specified as NodePath before setting this RNNavMesh up.\n
 * The owner object is, typically, a stationary/static object.\n
 * An "update" task should call this RNNavMesh update() method to allow the
 * RNCrowdAgent(s) (crowd agents), which are added to it, to head towards their
 * targets.\n
 * \note A model can "own" many RNNavMesh(es), so the typical pattern is to
 * have a common parent (ie reference) NodePath to which both model and its
 * RNNavMesh(es) are reparented. A RNNavMesh will be reparented to the default
 * reference node on creation (see RNNavMeshManager).
 *
 * > **RNNavMesh text parameters**:
 * param | type | default | note
 * ------|------|---------|-----
 * | *navmesh_type*					|single| *solo* | values: solo,tile,obstacle
 * | *cell_size*					|single| 0.3 | -
 * | *cell_height*					|single| 0.2 | -
 * | *agent_height*					|single| 2.0 | -
 * | *agent_radius*					|single| 0.6 | -
 * | *agent_max_climb*				|single| 0.9 | -
 * | *agent_max_slope*				|single| 45.0 | -
 * | *region_min_size*				|single| 8 | -
 * | *region_merge_size*			|single| 20 | -
 * | *partition_type*				|single| *watershed* | values: watershed,monotone,layer
 * | *edge_max_len*					|single| 12.0 | -
 * | *edge_max_error*				|single| 1.3 | -
 * | *verts_per_poly*				|single| 6.0 | -
 * | *detail_sample_dist*			|single| 6.0 | -
 * | *detail_sample_max_error*		|single| 1.0 | -
 * | *build_all_tiles*				|single| *false* | -
 * | *max_tiles*					|single| 128 | -
 * | *max_polys_per_tile*			|single| 32768 | -
 * | *tile_size*					|single| 32 | -
 * | *area_flags_cost*				|multiple| - | each one specified as "area_type@flag1[:flag2...:flagN]@cost" note: flags are or-ed
 * | *crowd_include_flags*			|single| - | specified as "flag1[:flag2...:flagN]" note: flags are or-ed
 * | *crowd_exclude_flags*			|single| - | specified as "flag1[:flag2...:flagN]" note: flags are or-ed
 * | *convex_volume*				|multiple| - | each one specified as "x1,y1,z1[:x2,y2,z2...:xN,yN,zN]@area_type"
 * | *offmesh_connection*			|multiple| - | each one specified as "xB,yB,zB:xE,yE,zE@bidirectional" with bidirectional=true,false
 *
 * \note parts inside [] are optional.\n
 */
class EXPCL_PANDARN RNNavMesh: public PandaNode
{
public:
	///convex volumes
	typedef Pair<ValueList<LPoint3f>,RNConvexVolumeSettings> PointListConvexVolumeSettings;
	///off mesh connections
	typedef Pair<ValueList<LPoint3f>,RNOffMeshConnectionSettings> PointPairOffMeshConnectionSettings;
	///obstacles
	typedef Pair<RNObstacleSettings, NodePath> Obstacle;
	///tester queries
	typedef ValueList<Pair<LPoint3f, unsigned char> > PointFlagList;

PUBLISHED:

	/**
	 * Equivalent to rnsup::NavMeshTypeEnum.
	 */
	enum RNNavMeshTypeEnum
	{
#ifndef CPPPARSER
		SOLO = rnsup::SOLO,
		TILE = rnsup::TILE,
		OBSTACLE = rnsup::OBSTACLE,
		NavMeshType_NONE = rnsup::NavMeshType_NONE
#else
		SOLO,TILE,OBSTACLE,NavMeshType_NONE
#endif //CPPPARSER
	};

	/**
	 * Equivalent to default rnsup::NavMeshPolyAreasEnum.
	 * \note You can add more area type values. Don't redefine these ones.
	 */
	enum RNNavMeshPolyAreasEnum
	{
#ifndef CPPPARSER
		POLYAREA_GROUND = rnsup::NAVMESH_POLYAREA_GROUND, //used by all walkable polys.
		POLYAREA_WATER = rnsup::NAVMESH_POLYAREA_WATER,
		POLYAREA_ROAD = rnsup::NAVMESH_POLYAREA_ROAD,
		POLYAREA_DOOR = rnsup::NAVMESH_POLYAREA_DOOR,
		POLYAREA_GRASS = rnsup::NAVMESH_POLYAREA_GRASS,
		POLYAREA_JUMP = rnsup::NAVMESH_POLYAREA_JUMP, //used by off mesh connections.
#else
		POLYAREA_GROUND,POLYAREA_WATER,POLYAREA_ROAD,
		POLYAREA_DOOR,POLYAREA_GRASS,POLYAREA_JUMP,
#endif //CPPPARSER
	};

	/**
	 * Equivalent to default rnsup::NavMeshPolyFlagsEnum.
	 * \note You can add more flag values. Don't redefine these ones.
	 */
	enum RNNavMeshPolyFlagsEnum
	{
#ifndef CPPPARSER
		POLYFLAGS_WALK		= rnsup::NAVMESH_POLYFLAGS_WALK, // Ability to walk (ground, grass, road)
		POLYFLAGS_SWIM		= rnsup::NAVMESH_POLYFLAGS_SWIM, // Ability to swim (water).
		POLYFLAGS_DOOR		= rnsup::NAVMESH_POLYFLAGS_DOOR, // Ability to move through doors.
		POLYFLAGS_JUMP		= rnsup::NAVMESH_POLYFLAGS_JUMP, // Ability to jump. Used by off mesh connections too.
		POLYFLAGS_DISABLED	= rnsup::NAVMESH_POLYFLAGS_DISABLED, // Disabled polygon.
		POLYFLAGS_ALL		= rnsup::NAVMESH_POLYFLAGS_ALL // All abilities.
#else
		POLYFLAGS_WALK,POLYFLAGS_SWIM,POLYFLAGS_DOOR,
		POLYFLAGS_JUMP,POLYFLAGS_DISABLED,POLYFLAGS_ALL
#endif //CPPPARSER
	};

	/**
	 * Equivalent to rnsup::NavMeshPartitionType.
	 */
	enum RNNavMeshPartitionType
	{
#ifndef CPPPARSER
		PARTITION_WATERSHED = rnsup::NAVMESH_PARTITION_WATERSHED,
		PARTITION_MONOTONE = rnsup::NAVMESH_PARTITION_MONOTONE,
		PARTITION_LAYERS = rnsup::NAVMESH_PARTITION_LAYERS,
#else
		PARTITION_WATERSHED,PARTITION_MONOTONE,PARTITION_LAYERS,
#endif //CPPPARSER
	};

	// To avoid interrogatedb warning.
#ifdef CPPPARSER
	virtual ~RNNavMesh();
#endif //CPPPARSER

	/**
	 * \name NAVMESH
	 */
	///@{
	int setup();
	int cleanup();
	INLINE bool is_setup();
	void update(float dt);
	///@}

	/**
	 * \name OWNER OBJECT
	 */
	///@{
	INLINE void set_owner_node_path(const NodePath& ownerObject);
	INLINE NodePath get_owner_node_path() const;
	///@}

	/**
	 * \name REFERENCE NODE
	 */
	///@{
	INLINE void set_reference_node_path(const NodePath& reference);
	///@}

	/**
	 * \name GENERAL PARAMETERS
	 */
	///@{
	INLINE LPoint3f get_nav_mesh_bounds_min() const;
	INLINE LPoint3f get_nav_mesh_bounds_max() const;
	///@}

	/**
	 * \name NAVMESH PARAMETERS
	 */
	///@{
	void set_nav_mesh_type_enum(RNNavMeshTypeEnum typeEnum);
	INLINE RNNavMeshTypeEnum get_nav_mesh_type_enum() const;
	void set_nav_mesh_settings(const RNNavMeshSettings& settings);
	INLINE RNNavMeshSettings get_nav_mesh_settings() const;
	INLINE void set_area_flags(int area, int oredFlags);
	INLINE int get_area_flags(int area) const;
	///@}

	/**
	 * \name CROWD PARAMETERS
	 */
	///@{
	void set_crowd_area_cost(int area, float cost);
	INLINE float get_crowd_area_cost(int area) const;
	void set_crowd_include_flags(int oredFlags);
	INLINE int get_crowd_include_flags() const;
	void set_crowd_exclude_flags(int oredFlags);
	INLINE int get_crowd_exclude_flags() const;
	///@}

	/**
	 * \name CONVEX VOLUMES
	 */
	///@{
	int add_convex_volume(const ValueList<LPoint3f>& points, int area);
	int remove_convex_volume(const LPoint3f& insidePoint);
	int set_convex_volume_settings(const LPoint3f& insidePoint,
		const RNConvexVolumeSettings& settings, float reductionFactor = 0.90);
	int set_convex_volume_settings(int ref, const RNConvexVolumeSettings& settings,
		float reductionFactor = 0.90);
	RNConvexVolumeSettings get_convex_volume_settings(
		const LPoint3f& insidePoint) const;
	RNConvexVolumeSettings get_convex_volume_settings(int ref) const;
	ValueList<LPoint3f> get_convex_volume_by_ref(int ref) const;
	INLINE int get_convex_volume(int index) const;
	INLINE int get_num_convex_volumes() const;
	MAKE_SEQ(get_convex_volumes, get_num_convex_volumes, get_convex_volume);
	///@}

	/**
	 * \name OFF MESH CONNECTIONS
	 */
	///@{
	int add_off_mesh_connection(const ValueList<LPoint3f>& points,
		bool bidirectional);
	int remove_off_mesh_connection(const LPoint3f& beginOrEndPoint);
	int set_off_mesh_connection_settings(const LPoint3f& beginOrEndPoint,
		const RNOffMeshConnectionSettings& settings);
	int set_off_mesh_connection_settings(int ref,
		const RNOffMeshConnectionSettings& settings);
	RNOffMeshConnectionSettings get_off_mesh_connection_settings(
		const LPoint3f& beginOrEndPoint) const;
	RNOffMeshConnectionSettings get_off_mesh_connection_settings(int ref) const;
	ValueList<LPoint3f> get_off_mesh_connection_by_ref(int ref) const;
	INLINE int get_off_mesh_connection(int index) const;
	INLINE int get_num_off_mesh_connections() const;
	MAKE_SEQ(get_off_mesh_connections, get_num_off_mesh_connections, get_off_mesh_connection);
	///@}

	/**
	 * \name TILE PARAMETERS
	 * (TILE and OBSTACLE types only)
	 */
	///@{
	void set_nav_mesh_tile_settings(const RNNavMeshTileSettings& settings);
	INLINE RNNavMeshTileSettings get_nav_mesh_tile_settings() const;
	LVecBase2i get_tile_indexes(const LPoint3f& pos);
	///@}

	/**
	 * \name TILES
	 * (TILE type only)
	 */
	///@{
	int build_tile(const LPoint3f& pos);
	int remove_tile(const LPoint3f& pos);
	int build_all_tiles();
	int remove_all_tiles();
	///@}

	/**
	 * \name OBSTACLES
	 * (OBSTACLE type only)
	 */
	///@{
	int add_obstacle(NodePath objectNP);
	int remove_obstacle(NodePath objectNP);
	NodePath get_obstacle_by_ref(int ref) const;
	INLINE int get_obstacle(int index) const;
	INLINE int get_num_obstacles() const;
	MAKE_SEQ(get_obstacles, get_num_obstacles, get_obstacle);
	int remove_all_obstacles();
	///@}

	/**
	 * \name CROWDAGENTS
	 */
	///@{
	int add_crowd_agent(NodePath crowdAgentNP);
	int remove_crowd_agent(NodePath crowdAgentNP);
	INLINE RNCrowdAgent *get_crowd_agent(int index) const;
	INLINE int get_num_crowd_agents() const;
	MAKE_SEQ(get_crowd_agents, get_num_crowd_agents, get_crowd_agent);
	INLINE RNCrowdAgent *operator [](int index) const;
	INLINE int size() const;
	///@}

	/**
	 * Equivalent to dtStraightPathOptions.
	 */
	enum RNStraightPathOptions
	{
#ifndef CPPPARSER
		NONE_CROSSINGS = 0,
		AREA_CROSSINGS = DT_STRAIGHTPATH_AREA_CROSSINGS,
		ALL_CROSSINGS = DT_STRAIGHTPATH_ALL_CROSSINGS
#else
		NONE_CROSSINGS,AREA_CROSSINGS,ALL_CROSSINGS
#endif //CPPPARSER
	};

	/**
	 * Equivalent to dtStraightPathFlags.
	 */
	enum RNStraightPathFlags
	{
#ifndef CPPPARSER
		START = DT_STRAIGHTPATH_START,
		END = DT_STRAIGHTPATH_END,
		OFFMESH_CONNECTION = DT_STRAIGHTPATH_OFFMESH_CONNECTION
#else
		START,END,OFFMESH_CONNECTION
#endif //CPPPARSER
	};

	/**
	 * \name TESTER QUERIES
	 */
	///@{
	ValueList<LPoint3f> path_find_follow(const LPoint3f& startPos,
		const LPoint3f& endPos);
	float path_find_follow_cost(const LPoint3f& startPos,
			const LPoint3f& endPos);
	PointFlagList path_find_straight(const LPoint3f& startPos,
		const LPoint3f& endPos, RNStraightPathOptions crossingOptions = NONE_CROSSINGS);
	LPoint3f ray_cast(const LPoint3f& startPos, const LPoint3f& endPos);
	float distance_to_wall(const LPoint3f& pos);
	///@}

	/**
	 * \name OUTPUT
	 */
	///@{
	void output(ostream &out) const;
	///@}

	/**
	 * \name DEBUG DRAWING
	 */
	///@{
	void enable_debug_drawing(NodePath debugCamera);
	void disable_debug_drawing();
	int toggle_debug_drawing(bool enable);
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
	typedef void (*UPDATECALLBACKFUNC)(PT(RNNavMesh));
	void set_update_callback(UPDATECALLBACKFUNC value);
	///@}
#endif //PYTHON_BUILD

public:
	/**
	 * \name C++ ONLY
	 * Library & support low level related methods.
	 */
	///@{
	inline rnsup::InputGeom* get_recast_input_geom() const;
	inline dtNavMesh* get_recast_nav_mesh() const;
	inline dtNavMeshQuery* get_recast_nav_mesh_query() const;
	inline dtCrowd* get_recast_crowd() const;
	dtTileCache* get_recast_tile_cache() const;
	inline rnsup::NavMeshType& get_nav_mesh_type() const;
	inline operator rnsup::NavMeshType&();
	///Unique ref producer.
	inline int unique_ref();
	///@}

protected:
	friend void unref_delete<RNNavMesh>(RNNavMesh*);
	friend class RNNavMeshManager;
	friend class RNCrowdAgent;

	RNNavMesh(const string& name = "NavMesh");
	virtual ~RNNavMesh();

private:
	///The owner object NodePath this RNNavMesh is associated to.
	NodePath mOwnerObject;
	///Current underlying NavMeshType.
	rnsup::NavMeshType* mNavMeshType;
	///Current underlying NavMeshType type.
	RNNavMeshTypeEnum mNavMeshTypeEnum;
	///Owner object's input geometry.
	rnsup::InputGeom* mGeom;
	///Build context.
	rnsup::BuildContext* mCtx;
	///Owner object's input geometry (mesh) name.
	string mMeshName;
	///The reference node path.
	NodePath mReferenceNP;
	///The reference node path for debug drawing.
	NodePath mReferenceDebugNP;
	///RNNavMesh's NavMeshSettings equivalent.
	RNNavMeshSettings mNavMeshSettings;
	///RNNavMesh's NavMeshTileSettings equivalent.
	RNNavMeshTileSettings mNavMeshTileSettings;
	///Area types with ability flags settings (see support/NavMeshType.h).
	rnsup::NavMeshPolyAreaFlags mPolyAreaFlags;
	///Area types with cost settings (see support/NavMeshType.h).
	rnsup::NavMeshPolyAreaCost mPolyAreaCost;
	///Crowd include & exclude flags settings (see library/DetourNavMeshQuery.h).
	int mCrowdIncludeFlags, mCrowdExcludeFlags;
	///Convex volumes (see support/ConvexVolumeTool.h).
	pvector<PointListConvexVolumeSettings> mConvexVolumes;
	///Off mesh connections (see support/OffMeshConnectionTool.h).
	pvector<PointPairOffMeshConnectionSettings> mOffMeshConnections;
	///Obstacles.
	pvector<Obstacle> mObstacles;
	///Crowd related data.
	//The RNCrowdAgents added to and handled by this RNNavMesh.
	pvector<RNCrowdAgent *> mCrowdAgents;
	int do_set_crowd_agent_params(RNCrowdAgent *crowdAgent,
			const RNCrowdAgentParams& params);
	int do_set_crowd_agent_target(RNCrowdAgent *crowdAgent,
			const LPoint3f& moveTarget);
	int do_set_crowd_agent_velocity(RNCrowdAgent *crowdAgent,
			const LVector3f& moveVelocity);

	///Used for saving underlying geometry (see TypedWritable API).
	rnsup::rcMeshLoaderObj mMeshLoader;

	///Tester tool.
	rnsup::NavMeshTesterTool mTesterTool;

	///Unique ref.
	int mRef;

	inline void do_reset();
	void do_initialize();
	void do_finalize();

	bool do_load_model_mesh(NodePath model,
		rnsup::rcMeshLoaderObj* meshLoader = NULL);
	void do_create_nav_mesh_type(rnsup::NavMeshType* navMeshType);
	bool do_build_navMesh();

	void do_add_crowd_agent_to_update_list(RNCrowdAgent *crowdAgent);
	bool do_add_crowd_agent_to_recast_update(RNCrowdAgent *crowdAgent,
			bool buildFromBam = false);
	void do_remove_crowd_agent_from_update_list(RNCrowdAgent *crowdAgent);
	void do_remove_crowd_agent_from_recast_update(RNCrowdAgent *crowdAgent);
	void do_set_crowd_agent_other_settings(
	RNCrowdAgent *crowdAgent, rnsup::CrowdTool* crowdTool);

	int do_get_convex_volume_from_point(const LPoint3f& insidePoint) const;
	int do_find_convex_volume_polys(int convexVolumeID, dtQueryFilter& filter,
		dtPolyRef* polys, int& npolys, const int MAX_POLYS, float reduceFactor) const;

	int do_get_off_mesh_connection_from_point(const LPoint3f& insidePoint) const;
	void do_find_off_mesh_connection_poly(int offMeshConnectionID,
			dtPolyRef* poly) const;

	int do_add_obstacle_to_recast(NodePath& objectNP, int index,
			bool buildFromBam = false);
	int do_remove_obstacle_from_recast(NodePath& objectNP, int obstacleRef);

#ifdef RN_DEBUG
	/// Recast debug node path.
	NodePath mDebugNodePath;
	/// Recast debug camera.
	NodePath mDebugCamera;
	/// DebugDrawers.
	rnsup::DebugDrawPanda3d* mDD;
	rnsup::DebugDrawMeshDrawer* mDDM;
	///Enable Draw update.
	bool mEnableDrawUpdate;
	/// Debug render with DebugDrawPanda3d.
	void do_debug_static_render();
	/// DebugDrawers.
	rnsup::DebugDrawPanda3d* mDDUnsetup;
	///Debug render when mNavMeshType is un-setup.
	void do_debug_static_render_unsetup();
#endif //RN_DEBUG

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
	RNNavMesh(const RNNavMesh&);
	RNNavMesh& operator=(const RNNavMesh&);

public:
	/**
	 * \name TypedWritable API
	 */
	///@{
	static void register_with_read_factory();
	virtual void write_datagram (BamWriter *manager, Datagram &dg) override;
	virtual int complete_pointers(TypedWritable **p_list, BamReader *manager) override;
	virtual void finalize(BamReader *manager);
	bool require_fully_complete() const;
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
		register_type(_type_handle, "RNNavMesh", PandaNode::get_class_type());
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

INLINE ostream &operator << (ostream &out, const RNNavMesh & navMesh);

///inline
#include "rnNavMesh.I"

#endif /* RNNAVMESH_H_ */
