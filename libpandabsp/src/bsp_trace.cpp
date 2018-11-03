/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file bsp_trace.cpp
 * @author Brian Lach
 * @date August 23, 2018
 */

#include "bsp_trace.h"

INLINE bool point_in_winding( const Winding& w, const dplane_t& plane, const vec_t* const point, vec_t epsilon = 0.0 )
{
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

INLINE bool test_point_against_surface( const LVector3 &point, dface_t *face, bspdata_t *data )
{
        Winding winding( *face, data );

        dplane_t plane;
        winding.getPlane( plane );

        vec3_t vpoint;
        vpoint[0] = point[0];
        vpoint[1] = point[1];
        vpoint[2] = point[2];
        return point_in_winding( winding, plane, vpoint );
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