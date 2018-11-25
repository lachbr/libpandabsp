/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file bsp_trace.h
 * @author Brian Lach
 * @date August 23, 2018
 */

#ifndef BSP_TRACE_H
#define BSP_TRACE_H

#include <winding.h>
#include <bsptools.h>

#include "mathlib/ssemath.h"

/**
 * Enumerates the BSP tree checking for intersections with any non TEX_SPECIAL faces.
 */
class FaceFinder : public BaseBSPEnumerator
{
public:
        FaceFinder( bspdata_t *data );
        virtual bool enumerate_leaf( int node_id, const Ray &ray, float start, float end, int context );
        virtual bool find_intersection( const Ray &ray );
        virtual bool enumerate_node( int node_id, const Ray &ray, float f, int context );

};

struct bspdata_t;

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

extern collbspdata_t *SetupCollisionBSPData( const bspdata_t *bspdata );

extern void CM_BoxTrace( const Ray &ray, int headnode, int brushmask,
                         bool compute_endpoint, const collbspdata_t *bspdata, Trace &trace );

#endif // BSP_TRACE_H