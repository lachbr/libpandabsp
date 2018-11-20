/**
* PANDA3D BSP LIBRARY
* Copyright (c) CIO Team. All rights reserved.
*
* @file bsp_trace.cpp
* @author Brian Lach
* @date August 23, 2018
*
* @desc This code is heavily derived from Source Engines's cmodel.cpp
*/

#include "bsp_trace.h"

#include <pstatTimer.h>
#include <pstatCollector.h>

static PStatCollector piw_collector( "BSP:Trace:PointInWinding" );
static PStatCollector ff_collector( "BSP:FaceFinder" );

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
        PStatTimer timer( ff_collector );

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

//==============================================================================================//
// This is the more efficient ray tracing algorithm, derived from Source Engine's cmodel.cpp
// The above code shouldn't be used for any real-time ray tracing.
// We might be well off just removing the code altogether.
//==============================================================================================//

#define NEVER_UPDATED -99999

template <bool IS_POINT>
void CM_ClipBoxToBrush( Trace *trace, const dbrush_t *brush )
{
        if ( !brush->numsides )
        {
                return;
        }

        const LVector3 &p1 = trace->start_pos;
        const LVector3 &p2 = trace->end_pos;
        int brush_contents = brush->contents;

        float enter_frac = NEVER_UPDATED;
        float leave_frac = 1.0;

        bool getout = false;
        bool startout = false;
        dbrushside_t *leadside = nullptr;

        float dist;

        dbrushside_t *side = nullptr;
        for (int i = 0; i < brush->numsides; i++ )
        {
                side = &trace->bspdata->dbrushsides[brush->firstside + i];
                dplane_t *plane = trace->bspdata->dplanes + side->planenum;
                LVector3 plane_normal( plane->normal[0], plane->normal[1], plane->normal[2] );

                if ( !IS_POINT )
                {
                        // general box case
                        // push the plane out appropriately for mins/maxs

                        dist = fabsf( plane_normal.dot( trace->extents ) );//DotProductAbs( plane_normal, trace->extents );
                        dist = plane->dist + dist;
                }
                else
                {
                        dist = plane->dist;
                        // don't trace rays against bevel planes
                        if ( side->bevel )
                        {
                                continue;
                        }
                }

                float d1 = p1.dot( plane_normal ) - dist;
                float d2 = p2.dot( plane_normal ) - dist;

                // if completely in front of face, no intersection
                if ( d1 > 0.0 )
                {
                        startout = true;

                        if ( d2 > 0.0 )
                        {
                                return;
                        }
                }
                else
                {
                        if ( d2 <= 0.0 )
                        {
                                continue;
                        }
                        getout = true;
                }

                // crosses face
                if ( d1 > d2 )
                {
                        // enter
                        float f = ( d1 - DIST_EPSILON );
                        if ( f < 0.f )
                        {
                                f = 0.f;
                        }
                        f = f / ( d1 - d2 );
                        if ( f > enter_frac )
                        {
                                enter_frac = f;
                                leadside = side;
                        }
                }
                else
                {	// leave
                        float f = ( d1 + DIST_EPSILON ) / ( d1 - d2 );
                        if ( f < leave_frac )
                        {
                                leave_frac = f;
                        }
                }
        }

        // when this happens, we entered the brush *after* leaving the previous brush.
        // Therefore, we're still outside!

        // NOTE: We only do this test against points because fractionleftsolid is
        // not possible to compute for brush sweeps without a *lot* more computation
        // So, client code will never get fractionleftsolid for box sweeps

        if ( IS_POINT && startout )
        {
                // Add a little sludge.  The sludge should already be in the fractionleftsolid
                // (for all intents and purposes is a leavefrac value) and enterfrac values.  
                // Both of these values have +/- DIST_EPSILON values calculated in.  Thus, I 
                // think the test should be against "0.0."  If we experience new "left solid"
                // problems you may want to take a closer look here!
//		if ((trace->fractionleftsolid - enterfrac) > -1e-6)
                if ( ( trace->fraction_left_solid - enter_frac ) > 0.0f )
                {
                        startout = false;
                } 
        }

        if ( !startout )
        {	// original point was inside brush
                trace->start_solid = true;
                // return starting contents
                trace->contents = brush_contents;

                if ( !getout )
                {
                        trace->all_solid = true;
                        trace->fraction = 0.0f;
                        trace->fraction_left_solid = 1.0f;
                }
                else
                {
                        // if leavefrac == 1, this means it's never been updated or we're in allsolid
                        // the allsolid case was handled above
                        if ( ( leave_frac != 1 ) && ( leave_frac > trace->fraction_left_solid ) )
                        {
                                trace->fraction_left_solid = leave_frac;

                                // This could occur if a previous trace didn't start us in solid
                                if ( trace->fraction <= leave_frac )
                                {
                                        trace->fraction = 1.0f;
                                        trace->surface = nullptr;
                                }
                        }
                }
                return;
        }

        // We haven't hit anything at all until we've left...
        if ( enter_frac < leave_frac )
        {
                if ( enter_frac > NEVER_UPDATED && enter_frac < trace->fraction )
                {
                        // WE HIT SOMETHING!!!!!
                        if ( enter_frac < 0 )
                                enter_frac = 0;
                        trace->fraction = enter_frac;
                        trace->plane = *( trace->bspdata->dplanes + leadside->planenum );
                        trace->surface = trace->bspdata->texinfo + side->texinfo;
                        trace->contents = brush_contents;
                }
        }
}

template <bool IS_POINT>
void CM_TraceToLeaf( Trace *trace, int leaf_idx, float start_frac, float end_frac )
{
        const dleaf_t *leaf = trace->bspdata->dleafs + leaf_idx;

        //
        // trace ray/box sweep against all brushes in this leaf
        //
        const int numleafbrushes = leaf->numleafbrushes;
        const int lastleafbrush = leaf->firstleafbrush + numleafbrushes;
        for ( int leafbrush = leaf->firstleafbrush; leafbrush < lastleafbrush; leafbrush++ )
        {
                int brushidx = trace->bspdata->dleafbrushes[leafbrush];

                const dbrush_t *brush = &trace->bspdata->dbrushes[brushidx];

                // only collide with objects you are interested in
                bool relevant_contents = ( brush->contents & trace->contents );
                if ( !relevant_contents )
                {
                        continue;
                }

                CM_ClipBoxToBrush<IS_POINT>( trace, brush );
                if ( !trace->fraction )
                {
                        return;
                }
        }

        if ( trace->start_solid )
        {
                return;
        }
}

template <bool IS_POINT>
void CM_RecursiveHullCheckImpl( Trace *trace, int num, const float p1f, const float p2f,
                                const LVector3 &p1, const LVector3 &p2 )
{
        if ( trace->fraction <= p1f )
        {
                // already hit something nearer
                return;
        }

        float t1 = 0, t2 = 0, offset = 0;
        float frac, frac2;
        float idist;
        LVector3 mid;
        int side;
        float midf;
        const dnode_t *node;

        // find the point distances to the separating plane
        // and the offset for the size of the box

        while ( num >= 0 )
        {
                node = trace->bspdata->dnodes + num;
                const dplane_t *plane = trace->bspdata->dplanes + node->planenum;
                LVector3 plane_normal( plane->normal[0], plane->normal[1], plane->normal[2] );
                int type = plane->type;
                float dist = plane->dist;

                if ( type < 3 )
                {
                        t1 = p1[type] - dist;
                        t2 = p2[type] - dist;
                        offset = trace->extents[type];
                }
                else
                {
                        t1 = p1.dot( plane_normal ) - dist;
                        t2 = p2.dot( plane_normal ) - dist;
                        if ( IS_POINT )
                        {
                                offset = 0.0;
                        }
                        else
                        {
                                offset = fabsf( plane_normal.dot( trace->extents ) );
                        }
                }

                // see which sides we need to consider
                if ( t1 > offset && t2 > offset )
                {
                        num = node->children[0];
                        continue;
                }
                if ( t1 < -offset && t2 < -offset )
                {
                        num = node->children[1];
                        continue;
                }
                break;
        }

        // if < 0, we are in a leaf node
        if ( num < 0 )
        {
                CM_TraceToLeaf<IS_POINT>( trace, ~num, p1f, p2f );
                return;
        }

        // put the crosspoint DIST_EPSILON pixels on the near side
        if ( t1 < t2 )
        {
                idist = 1.0 / ( t1 - t2 );
                side = 1;
                frac2 = ( t1 + offset + DIST_EPSILON ) * idist;
                frac = ( t1 - offset - DIST_EPSILON ) * idist;
        }
        else if ( t1 > t2 )
        {
                idist = 1.0 / ( t1 - t2 );
                side = 0;
                frac2 = ( t1 - offset - DIST_EPSILON ) * idist;
                frac = ( t1 + offset + DIST_EPSILON ) * idist;
        }
        else
        {
                side = 0;
                frac = 1;
                frac2 = 0;
        }

        // move up to the node
        frac = clamp( frac, 0.0f, 1.0f );
        midf = p1f + ( p2f - p1f ) * frac;
        VectorLerp( p1, p2, frac, mid );

        CM_RecursiveHullCheckImpl<IS_POINT>( trace, node->children[side], p1f, midf, p1, mid );

        // go past the node
        frac2 = clamp( frac2, 0.0f, 1.0f );
        midf = p1f + ( p2f - p1f ) * frac2;
        VectorLerp( p1, p2, frac2, mid );

        CM_RecursiveHullCheckImpl<IS_POINT>( trace, node->children[side ^ 1], midf, p2f, mid, p2 );
}

void CM_RecursiveHullCheck( Trace *trace, int headnode, const float p1f, const float p2f )
{

        if ( trace->is_point )
        {
                CM_RecursiveHullCheckImpl<true>( trace, headnode, p1f, p2f,
                                                 trace->start_pos, trace->end_pos );
        }
        else
        {
                CM_RecursiveHullCheckImpl<false>( trace, headnode, p1f, p2f,
                                                  trace->start_pos, trace->end_pos );
        }
}

void CM_ComputeTraceEndpoints( const Ray &ray, Trace *trace )
{
        LVector3 start = ray.start + ray.start_offset;

        if ( trace->fraction == 1.0 )
        {
                trace->end_pos = start + ray.delta;
        }
        else
        {
                VectorMA( start, trace->fraction, ray.delta, trace->end_pos );
        }

        if ( trace->fraction_left_solid == 0 )
        {
                trace->start_pos = start;
        }
        else
        {
                if ( trace->fraction_left_solid == 1.0 )
                {
                        trace->start_solid = trace->all_solid = true;
                        trace->fraction = 0.0;
                        trace->end_pos = start;
                }

                VectorMA( start, trace->fraction_left_solid, ray.delta, trace->start_pos );
        }
}

static PStatCollector bt_collector( "BSP:CM_BoxTrace" );

/**
 * Traces a line/ray along the BSP tree.
 * Starts at the specified node, only intersects with specified brush contents mask.
 * Results of the trace are filled in, use Trace::has_hit() to see if the line intersected something.
 */
void CM_BoxTrace( const Ray &ray, int headnode, int brushmask, bool compute_endpoint, const bspdata_t *bspdata, Trace &trace )
{
        PStatTimer timer( bt_collector );

        trace.contents = brushmask;
        trace.start_pos = ray.start;
        trace.end_pos = ray.start + ray.delta;
        trace.extents = ray.extents;
        trace.mins = -ray.extents;
        trace.maxs = ray.extents;
        trace.is_point = ray.is_ray;
        trace.bspdata = (bspdata_t *)bspdata;

        // general sweeping through the world
        CM_RecursiveHullCheck( &trace, headnode, 0, 1 );

        if ( compute_endpoint )
        {
                CM_ComputeTraceEndpoints( ray, &trace );
        }
}