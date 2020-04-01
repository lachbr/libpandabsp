/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file bsp_trace.h
 * @author Brian Lach
 * @date August 23, 2018
 */

#ifndef BSP_TRACE_H
#define BSP_TRACE_H

#ifndef CPPPARSER

#include <winding.h>
#include <bsptools.h>
#include "raytrace.h"

struct bspdata_t;

/**
 * Enumerates the BSP tree checking for intersections with any non TEX_SPECIAL faces.
 */
class EXPCL_PANDABSP FaceFinder : public BaseBSPEnumerator
{
public:
        FaceFinder( bspdata_t *data );
        virtual bool enumerate_leaf( int node_id, const Ray &ray, float start, float end, int context );
        virtual bool find_intersection( const Ray &ray );
        virtual bool enumerate_node( int node_id, const Ray &ray, float f, int context );

};

struct cboxbrush_t
{
        LVector3 mins;
        LVector3 maxs;

        fltx4 ssemins;
        fltx4 ssemaxs;

        unsigned short surface_indices[6];

        int is_box;
};

struct collbspdata_t
{
        const bspdata_t *bspdata;
        cboxbrush_t boxbrushes[MAX_MAP_BRUSHES];
};

extern EXPCL_PANDABSP collbspdata_t *SetupCollisionBSPData( const bspdata_t *bspdata );

extern EXPCL_PANDABSP void CM_BoxTrace( const Ray &ray, int headnode, int brushmask,
                         bool compute_endpoint, const collbspdata_t *bspdata, Trace &trace );

class BSPLoader;

enum
{
	TRACETYPE_WORLD = 1 << 0,
	TRACETYPE_DETAIL = 1 << 1,
};

class EXPCL_PANDABSP BSPTrace : public ReferenceCount
{
public:
	BSPTrace( BSPLoader *loader );

	void add_dmodel( const dmodel_t *model, unsigned int mask );

	INLINE RayTraceScene *get_scene() const
	{
		return _scene;
	}

	INLINE const dface_t *lookup_dface( int geom_id )
	{
		int idx = _dface_map.find( geom_id );
		if ( idx == -1 )
			return nullptr;
		return _dface_map.get_data( idx );
	}

	void clear();

private:
	PT( RayTraceScene ) _scene;
	pvector<PT( RayTraceGeometry )> _geom_handles;
	typedef SimpleHashMap<int, const dface_t *, int_hash> RT_DFaceMap;
	RT_DFaceMap _dface_map;

	BSPLoader *_loader;
};

#else

class BSPTrace;

#endif

#endif // BSP_TRACE_H
