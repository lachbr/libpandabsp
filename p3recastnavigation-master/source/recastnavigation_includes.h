/**
 * \file recastnavigation_includes.h
 *
 * \date 2016-03-19
 * \author consultit
 */

#ifndef RECASTNAVIGATION_INCLUDES_H_
#define RECASTNAVIGATION_INCLUDES_H_

#ifdef CPPPARSER
//Panda3d interrogate fake declarations

namespace rnsup
{
	struct InputGeom;
	struct NavMeshType;
	struct NavMeshSettings;
	struct NavMeshTileSettings;
	struct BuildContext;
	struct NavMeshPolyAreaFlags;
	struct NavMeshPolyAreaCost;
	struct DebugDrawPanda3d;
	struct DebugDrawMeshDrawer;
	struct NavMeshTesterTool;
	struct rcMeshLoaderObj;
}

struct dtNavMesh;
struct dtNavMeshQuery;
struct dtCrowd;
struct dtTileCache;
struct dtCrowdAgentParams;
typedef unsigned int dtObstacleRef;

#endif //CPPPARSER

#endif /* RECASTNAVIGATION_INCLUDES_H_ */
