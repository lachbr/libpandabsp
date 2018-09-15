# recastnavigation tests
ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = \
	-I$(srcdir)/../../source \
	-I$(srcdir)/../../source/library \
	-I$(srcdir)/../../source/support \
	-I/usr/include/panda3d -I/usr/include/eigen3 \
	-DRN_DEBUG=

AM_CXXFLAGS = -O0 -g3 -Wall -Wno-reorder -fmessage-length=0 -std=c++11
AM_LDFLAGS = -L/usr/lib/x86_64-linux-gnu/panda3d

LDADD = -lp3framework -lpanda -lpandaexpress -lp3dtool -lp3dtoolconfig \
		-lp3pystub -lXxf86dga -lpython2.7 -lpandaphysics -lpthread

noinst_PROGRAMS = basic callback_test test1 test2

BUILT_SOURCES = data.h

substDataDir = sed -e 's|@sampledatadir[@]|$(srcdir)/../data|g'

data.h: $(srcdir)/data.h.in Makefile
	$(substDataDir) $(srcdir)/$@.in > $@
	
EXTRA_DIST = data.h.in

common_sources = \
	$(srcdir)/../../source/rnCrowdAgent.cxx \
	$(srcdir)/../../source/rnNavMesh.cxx \
	$(srcdir)/../../source/rnNavMeshManager.cxx \
	$(srcdir)/../../source/rnTools.cxx \
	$(srcdir)/../../source/library/DebugDraw.cpp \
	$(srcdir)/../../source/library/DetourAlloc.cpp \
	$(srcdir)/../../source/library/DetourAssert.cpp \
	$(srcdir)/../../source/library/DetourCommon.cpp \
	$(srcdir)/../../source/library/DetourCrowd.cpp \
	$(srcdir)/../../source/library/DetourDebugDraw.cpp \
	$(srcdir)/../../source/library/DetourLocalBoundary.cpp \
	$(srcdir)/../../source/library/DetourNavMeshBuilder.cpp \
	$(srcdir)/../../source/library/DetourNavMesh.cpp \
	$(srcdir)/../../source/library/DetourNavMeshQuery.cpp \
	$(srcdir)/../../source/library/DetourNode.cpp \
	$(srcdir)/../../source/library/DetourObstacleAvoidance.cpp \
	$(srcdir)/../../source/library/DetourPathCorridor.cpp \
	$(srcdir)/../../source/library/DetourPathQueue.cpp \
	$(srcdir)/../../source/library/DetourProximityGrid.cpp \
	$(srcdir)/../../source/library/DetourTileCacheBuilder.cpp \
	$(srcdir)/../../source/library/DetourTileCache.cpp \
	$(srcdir)/../../source/library/RecastAlloc.cpp \
	$(srcdir)/../../source/library/RecastAssert.cpp \
	$(srcdir)/../../source/library/RecastArea.cpp \
	$(srcdir)/../../source/library/RecastContour.cpp \
	$(srcdir)/../../source/library/Recast.cpp \
	$(srcdir)/../../source/library/RecastDebugDraw.cpp \
	$(srcdir)/../../source/library/RecastDump.cpp \
	$(srcdir)/../../source/library/RecastFilter.cpp \
	$(srcdir)/../../source/library/RecastLayers.cpp \
	$(srcdir)/../../source/library/RecastMesh.cpp \
	$(srcdir)/../../source/library/RecastMeshDetail.cpp \
	$(srcdir)/../../source/library/RecastRasterization.cpp \
	$(srcdir)/../../source/library/RecastRegion.cpp \
	$(srcdir)/../../source/support/ChunkyTriMesh.cpp \
	$(srcdir)/../../source/support/ConvexVolumeTool.cpp \
	$(srcdir)/../../source/support/CrowdTool.cpp \
	$(srcdir)/../../source/support/DebugInterfaces.cpp \
	$(srcdir)/../../source/support/fastlz.c \
	$(srcdir)/../../source/support/InputGeom.cpp \
	$(srcdir)/../../source/support/MeshLoaderObj.cpp \
	$(srcdir)/../../source/support/NavMeshTesterTool.cpp \
	$(srcdir)/../../source/support/NavMeshType.cpp \
	$(srcdir)/../../source/support/NavMeshType_Obstacle.cpp \
	$(srcdir)/../../source/support/NavMeshType_Solo.cpp \
	$(srcdir)/../../source/support/NavMeshType_Tile.cpp \
	$(srcdir)/../../source/support/OffMeshConnectionTool.cpp \
	$(srcdir)/../../source/support/PerfTimer.cpp

#basic
basic_SOURCES = \
	basic.cpp

nodist_basic_SOURCES = $(common_sources)

#callback_test
callback_test_SOURCES = \
	callback_test.cpp

nodist_callback_test_SOURCES = $(common_sources)

#test1
test1_SOURCES = \
	test1.cpp

nodist_test1_SOURCES = $(common_sources)

#test2
test2_SOURCES = \
	test2.cpp

nodist_test2_SOURCES = $(common_sources)

CLEANFILES = data.h