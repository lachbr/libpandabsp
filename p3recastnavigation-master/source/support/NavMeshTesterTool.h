//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

/**
 * \file NavMeshTesterTool.h
 *
 * \date 2016-05-14
 * \author Mikko Mononen (modified by consultit)
 */

#ifndef NAVMESHTESTERTOOL_H
#define NAVMESHTESTERTOOL_H

#include "NavMeshType.h"
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>

namespace rnsup
{

class NavMeshTesterTool : public NavMeshTypeTool
{
	NavMeshType* m_sample;
	
	dtNavMesh* m_navMesh;
	dtNavMeshQuery* m_navQuery;

	dtQueryFilter* m_filter;
	dtQueryFilter m_defaultFilter;

	dtStatus m_pathFindStatus;

public:
	enum ToolMode
	{
		TOOLMODE_PATHFIND_FOLLOW,
		TOOLMODE_PATHFIND_STRAIGHT,
		TOOLMODE_PATHFIND_SLICED,
		TOOLMODE_RAYCAST,
		TOOLMODE_DISTANCE_TO_WALL,
		TOOLMODE_FIND_POLYS_IN_CIRCLE,
		TOOLMODE_FIND_POLYS_IN_SHAPE,
		TOOLMODE_FIND_LOCAL_NEIGHBOURHOOD,
	};

private:
	ToolMode m_toolMode;

	int m_straightPathOptions;
	
	static const int MAX_POLYS = 256;
	static const int MAX_SMOOTH = 2048;
	
	dtPolyRef m_startRef;
	dtPolyRef m_endRef;
	dtPolyRef m_polys[MAX_POLYS];
	dtPolyRef m_parent[MAX_POLYS];
	int m_npolys;
	float m_straightPath[MAX_POLYS*3];
	unsigned char m_straightPathFlags[MAX_POLYS];
	dtPolyRef m_straightPathPolys[MAX_POLYS];
	int m_nstraightPath;
	float m_polyPickExt[3];
	float m_smoothPath[MAX_SMOOTH*3];
	int m_nsmoothPath;
	float m_queryPoly[4*3];
	float m_totalCost;  //total cost of the last path found,
						//TOOLMODE_PATHFIND_FOLLOW only.

	static const int MAX_RAND_POINTS = 64;
	float m_randPoints[MAX_RAND_POINTS*3];
	int m_nrandPoints;
	bool m_randPointsInCircle;
	
	float m_spos[3];
	float m_epos[3];
	float m_hitPos[3];
	float m_hitNormal[3];
	bool m_hitResult;
	float m_distanceToWall;
	float m_neighbourhoodRadius;
	float m_randomRadius;
	bool m_sposSet;
	bool m_eposSet;

	int m_pathIterNum;
	dtPolyRef m_pathIterPolys[MAX_POLYS]; 
	int m_pathIterPolyCount;
	float m_prevIterPos[3], m_iterPos[3], m_steerPos[3], m_targetPos[3];
	
	static const int MAX_STEER_POINTS = 10;
	float m_steerPoints[MAX_STEER_POINTS*3];
	int m_steerPointCount;
	
public:
	NavMeshTesterTool();

	virtual int type() { return TOOL_NAVMESH_TESTER; }
	virtual void init(NavMeshType* sample){}
	virtual void init(NavMeshType* sample, dtQueryFilter* filter);
	virtual void reset();
//	virtual void handleMenu();
	virtual void handleClick(const float* s, const float* p, bool shift);
	virtual void handleToggle();
	virtual void handleStep();
	virtual void handleUpdate(const float dt);
	virtual void handleRender(duDebugDraw& dd);
//	virtual void handleRenderOverlay(double* proj, double* model, int* view);

	void recalc();
	void drawAgent(const float* pos, float r, float h, float c, const unsigned int col);

	//
	void setToolMode(ToolMode mode)
	{
		m_toolMode = mode;
	}
	void setStartEndPos(const float* s, const float* e);

	//TOOLMODE_PATHFIND_FOLLOW
	float *getSmoothPath()
	{
		return m_smoothPath;
	}
	int getNumSmoothPath()
	{
		return m_nsmoothPath;
	}
	float getTotalCost()
	{
		return m_totalCost;
	}
	//TOOLMODE_PATHFIND_STRAIGHT || TOOLMODE_PATHFIND_SLICED
	void setStraightOptions(int options)
	{
		m_straightPathOptions = options;
	}
	float *getStraightPath()
	{
		return m_straightPath;
	}
	unsigned char* getStraightPathFlags()
	{
		return m_straightPathFlags;
	}
	int getNumStraightPath()
	{
		return m_nstraightPath;
	}
	//TOOLMODE_RAYCAST
	bool getHitResult()
	{
		return m_hitResult;
	}
	float *getHitPos()
	{
		return m_hitPos;
	}
	//TOOLMODE_DISTANCE_TO_WALL
	float getDistanceToWall()
	{
		return m_distanceToWall;
	}
};

} // namespace rnsup

#endif // NAVMESHTESTERTOOL_H
