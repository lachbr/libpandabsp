/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file bsp_trace.cpp
 * @author Brian Lach
 * @date August 23, 2018
 */

#include "bsp_trace.h"

#include <pstatTimer.h>
#include <pstatCollector.h>

static PStatCollector piw_collector( "BSP:Trace:PointInWinding" );

#define PIW_PVEC // Use panda vectors for point_in_winding ( eigen sse )

#ifdef PIW_PVEC
INLINE bool point_in_winding( const Winding& w, const dplane_t& plane,
                              const LVector3 &v_point, vec_t epsilon = 0.0 )
{
        PStatTimer timer( piw_collector );

        int				numpoints;
        int				x;
        LVector3	        delta;
        vec_t			dist;

        // Move these into panda vectors to get Eigen's SSE optimizations
        LVector3 v_normal;
        LVector3 v_planenormal( plane.normal[0], plane.normal[1], plane.normal[2] );

        numpoints = w.m_NumPoints;

        for ( x = 0; x < numpoints; x++ )
        {
                LVector3 v_wpoint( w.m_Points[x][0], w.m_Points[x][1], w.m_Points[x][2] );

                int idx = ( x + 1 ) % numpoints;
                LVector3 v_owpoint( w.m_Points[idx][0], w.m_Points[idx][1], w.m_Points[idx][2] );

                delta = v_owpoint - v_wpoint;
                v_normal = delta.cross( v_planenormal );
                dist = v_point.dot( v_normal ) - v_wpoint.dot( v_normal );

                if ( dist < 0.0
                     && ( epsilon == 0.0 || dist * dist > epsilon * epsilon * v_normal.dot( v_normal ) ) )
                {
                        return false;
                }
        }

        return true;
}

#else

bool point_in_winding( const Winding& w, const dplane_t& plane, const vec_t* const point, vec_t epsilon = 0.0 )
{
        PStatTimer timer( piw_collector );

        int				numpoints;
        int				x;
        vec3_t			delta;
        vec3_t			normal;
        vec_t			dist;

        numpoints = w.m_NumPoints;

        for ( x = 0; x < numpoints; x++ )
        {
                VectorSubtract( w.m_Points[( x + 1 ) % numpoints], w.m_Points[x], delta );
                CrossProduct( delta, plane.normal, normal );
                dist = DotProduct( point, normal ) - DotProduct( w.m_Points[x], normal );

                if ( dist < 0.0
                     && ( epsilon == 0.0 || dist * dist > epsilon * epsilon * DotProduct( normal, normal ) ) )
                {
                        return false;
                }
        }

        return true;
}

#endif

INLINE bool test_point_against_surface( const LVector3 &point, dface_t *face, bspdata_t *data )
{
        Winding winding( *face, data );

        dplane_t plane;
        winding.getPlane( plane );

#ifdef PIW_PVEC
        return point_in_winding( winding, plane, point );
#else
        vec3_t vpoint;
        VectorCopy( point, vpoint );
        return point_in_winding( winding, plane, vpoint );
#endif
}

FaceFinder::FaceFinder( bspdata_t *data ) :
        BaseBSPEnumerator( data )
{
}

bool FaceFinder::enumerate_leaf( int node_id, const Ray &ray, float start, float end, int context )
{
        return true;
}

bool FaceFinder::find_intersection( const Ray &ray )
{
        return !enumerate_nodes_along_ray( ray, this, 0 );
}

bool FaceFinder::enumerate_node( int node_id, const Ray &ray, float f, int context )
{
        LVector3 pt;
        VectorMA( ray.start, f, ray.delta, pt );

        dnode_t *node = &data->dnodes[node_id];

        for ( int i = 0; i < node->numfaces; i++ )
        {
                dface_t *face = &data->dfaces[node->firstface + i];

                texinfo_t *tex = &data->texinfo[face->texinfo];
                if ( !( tex->flags & TEX_SPECIAL ) )
                {
                        if ( test_point_against_surface( pt, face, data ) )
                        {
                                return false;
                        }
                }
        }

        return true;
}