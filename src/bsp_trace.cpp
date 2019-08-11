/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file bsp_trace.cpp
 * @author Brian Lach
 * @date August 23, 2018
 *
 * @desc This code is heavily derived from Source Engines's cmodel.cpp
 */

#include "bsp_trace.h"
#include "bsploader.h"

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

#define FLTX4_PRINT(flt) flt.m128_f32[0] << " " << flt.m128_f32[1] << " " << flt.m128_f32[2] << " " << flt.m128_f32[3]

// SIMD accelerated ray tracing for box brushes

const fltx4 Four_DistEpsilons = { DIST_EPSILON,DIST_EPSILON,DIST_EPSILON,DIST_EPSILON };
//const fltx4 g_CubeFaceIndex0 = { 0, 1, 2, -1 };
//const fltx4 g_CubeFaceIndex1 = { 3, 4, 5, -1 };
const int32_t ALIGN_16BYTE g_CubeFaceIndex0[4] = { 0, 1, 2, -1 };
const int32_t ALIGN_16BYTE g_CubeFaceIndex1[4] = { 3, 4, 5, -1 };

bool IntersectRayWithBoxBrush( Trace *trace, const dbrush_t *brush, const cboxbrush_t *bbrush )
{
        // compute the mins/maxs of the box expanded by the ray extents
        // relocate the problem so that the ray start is at the origin
        fltx4 offsetmins = SubSIMD( bbrush->ssemins, trace->sse_start );
        fltx4 offsetmaxs = SubSIMD( bbrush->ssemaxs, trace->sse_start );
        fltx4 offsetmins_exp = SubSIMD( offsetmins, trace->sse_extents );
        fltx4 offsetmaxs_exp = AddSIMD( offsetmaxs, trace->sse_extents );

        // check to see if both the origin (start point) and the end point (delta) are on the front side
        // of any of the box sides - if so there can be no intersection
        fltx4 startout_mins = CmpLtSIMD( Four_Zeros, offsetmins_exp );
        fltx4 endout_mins = CmpLtSIMD( trace->sse_delta, offsetmins_exp );
        fltx4 mins_mask = AndSIMD( startout_mins, endout_mins );
        fltx4 startout_maxs = CmpGtSIMD( Four_Zeros, offsetmaxs_exp );
        fltx4 endout_maxs = CmpGtSIMD( trace->sse_delta, offsetmaxs_exp );
        fltx4 maxs_mask = AndSIMD( startout_maxs, endout_maxs );
        if ( IsAnyNegative( SetWToZeroSIMD( OrSIMD( mins_mask, maxs_mask ) ) ) )
        {
                return false;
        }
        
        fltx4 cross_plane = OrSIMD( XorSIMD( startout_mins, endout_mins ), XorSIMD( startout_maxs, endout_maxs ) );
        // now build the per-axis interval of t for intersections
        fltx4 tmins = MulSIMD( offsetmins_exp, trace->sse_inv_delta );
        fltx4 tmaxs = MulSIMD( offsetmaxs_exp, trace->sse_inv_delta );
        // now sort the interval per axis
        fltx4 mint = MinSIMD( tmins, tmaxs );
        fltx4 maxt = MaxSIMD( tmins, tmaxs );
        // only axes where we cross a plane are relevant
        mint = MaskedAssign( cross_plane, mint, Four_Negative_FLT_MAX );
        maxt = MaskedAssign( cross_plane, maxt, Four_FLT_MAX );

        // now find the intersection of the intervals on all axes
        fltx4 first_out = FindLowestSIMD3( maxt );
        fltx4 last_in = FindHighestSIMD3( mint );
        // NOTE: this is really a scalar quantity [t0, t1] == [last_in, last_out]
        first_out = MinSIMD( first_out, Four_Ones );
        last_in = MaxSIMD( last_in, Four_Zeros );

        // if the final interval is valid last_in < first_out, check for separation
        fltx4 separation = CmpGtSIMD( last_in, first_out );

        if ( IsAllZeros( separation ) )
        {
                bool start_out = IsAnyNegative( SetWToZeroSIMD( OrSIMD( startout_mins, startout_maxs ) ) );
                offsetmins_exp = SubSIMD( offsetmins_exp, Four_DistEpsilons );
                offsetmaxs_exp = AddSIMD( offsetmaxs_exp, Four_DistEpsilons );

                fltx4 tmins = MulSIMD( offsetmins_exp, trace->sse_inv_delta );
                fltx4 tmaxs = MulSIMD( offsetmaxs_exp, trace->sse_inv_delta );

                fltx4 minface0 = LoadAlignedSIMD( (float *)g_CubeFaceIndex0 );//g_CubeFaceIndex0;
                fltx4 minface1 = LoadAlignedSIMD( (float *)g_CubeFaceIndex1 ); //g_CubeFaceIndex1;
                fltx4 face_mask = CmpLeSIMD( tmins, tmaxs );
                fltx4 mint = MinSIMD( tmins, tmaxs );
                fltx4 maxt = MaxSIMD( tmins, tmaxs );
                fltx4 face_id = MaskedAssign( face_mask, minface0, minface1 );
                // only axes where we cross a plane are relevant
                mint = MaskedAssign( cross_plane, mint, Four_Negative_FLT_MAX );
                maxt = MaskedAssign( cross_plane, maxt, Four_FLT_MAX );

                fltx4 tmp_first_out = FindLowestSIMD3( maxt );

                // implement FindHighest of 3, butuse intermediate masks to find the
                // corresponding index in face_id to the highest at the sametime
                fltx4 cmp_one = RotateLeft( mint );
                face_mask = CmpGtSIMD( mint, cmp_one );
                // cmp_one is [y, z, G, x]
                fltx4 max_xy = MaxSIMD( mint, cmp_one );
                fltx4 face_rot = RotateLeft( face_id );
                fltx4 face_id_xy = MaskedAssign( face_mask, face_id, face_rot );
                // max_xy = [max(x, y), ... ]
                cmp_one = RotateLeft2( mint );
                face_rot = RotateLeft2( face_id );
                // cmp_one is [z, G, x, y]
                face_mask = CmpGtSIMD( max_xy, cmp_one );
                fltx4 max_xyz = MaxSIMD( max_xy, cmp_one );
                face_id = MaskedAssign( face_mask, face_id_xy, face_rot );
                fltx4 tmp_last_in = SplatXSIMD( max_xyz );

                fltx4 first_out = MinSIMD( tmp_first_out, Four_Ones );
                fltx4 last_in = MaxSIMD( tmp_last_in, Four_Zeros );
                fltx4 separation = CmpGtSIMD( last_in, first_out );
                if ( IsAllZeros( separation ) )
                {
                        uint32_t face_idx = SubInt( face_id, 0 );
                        float t1 = SubFloat( last_in, 0 );

                        if ( start_out && trace->is_point && trace->fraction_left_solid > t1 )
                                start_out = false;

                        if ( !start_out )
                        {
                                float t2 = SubFloat( first_out, 0 );
                                trace->start_solid = true;
                                trace->hit_contents = brush->contents;
                                if ( t2 >= 1.0f )
                                {
                                        trace->all_solid = true;
                                        trace->fraction = 0.0f;
                                }
                                else if ( t2 > trace->fraction_left_solid )
                                {
                                        trace->fraction_left_solid = t2;
                                        if ( trace->fraction <= t2 )
                                        {
                                                trace->fraction = 1.0f;
                                                trace->surface = nullptr;
                                        }
                                }
                        }
                        else
                        {
                                static const int signbits[3] = { 1, 2, 4 };
                                if ( t1 < trace->fraction )
                                {
                                        trace->fraction = t1;
                                        VectorCopy( vec3_origin, trace->plane.normal );
                                        trace->surface = (texinfo_t *)&trace->bspdata->bspdata->texinfo[bbrush->surface_indices[face_idx]];

                                        if ( face_idx >= 3 )
                                        {
                                                face_idx -= 3;
                                                trace->plane.dist = bbrush->maxs[face_idx];
                                                trace->plane.normal[face_idx] = 1.0f;
                                        }
                                        else
                                        {
                                                trace->plane.dist = -bbrush->mins[face_idx];
                                                trace->plane.normal[face_idx] = -1.0f;
                                        }
                                        trace->plane.type = (planetypes)face_idx;
                                        trace->hit_contents = brush->contents;
                                        return true;
                                }
                        }
                }
        }
        return false;
}

#define NEVER_UPDATED -99999

template <bool IS_POINT>
void CM_ClipBoxToBrush( Trace *trace, const dbrush_t *brush, int brush_idx )
{
        // special SIMD accelerated case for box brushes ( 6 sides and axis-aligned )
        if ( trace->bspdata->boxbrushes[brush_idx].is_box )
        {
                cboxbrush_t *bbrush = trace->bspdata->boxbrushes + brush_idx;
                IntersectRayWithBoxBrush( trace, brush, bbrush );
                return;
        }

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
        const dbrushside_t *leadside = nullptr;

        float dist;

        const dbrushside_t *side = &trace->bspdata->bspdata->dbrushsides[brush->firstside];
        for (const dbrushside_t * const sidelimit = side + brush->numsides; side < sidelimit; side++)
        {
                const dplane_t *plane = trace->bspdata->bspdata->dplanes + side->planenum;
                LVector3 plane_normal( plane->normal[0], plane->normal[1], plane->normal[2] );

                if ( !IS_POINT )
                {
                        // general box case
                        // push the plane out appropriately for mins/maxs

                        dist = DotProductAbs( plane_normal, trace->extents );
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
                trace->hit_contents = brush_contents;

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
                        trace->plane = *( trace->bspdata->bspdata->dplanes + leadside->planenum );
                        trace->surface = (texinfo_t *)trace->bspdata->bspdata->texinfo + side->texinfo;
                        trace->hit_contents = brush_contents;
                }
        }
}

template <bool IS_POINT>
void CM_TraceToLeaf( Trace *trace, int leaf_idx, float start_frac, float end_frac )
{
        const dleaf_t *leaf = trace->bspdata->bspdata->dleafs + leaf_idx;

        bool simd_loaded = false;

        //
        // trace ray/box sweep against all brushes in this leaf
        //
        for ( int leafbrush = 0; leafbrush < leaf->numleafbrushes; leafbrush++ )
        {
                int lbidx = leaf->firstleafbrush + leafbrush;
                int brushidx = trace->bspdata->bspdata->dleafbrushes[lbidx];

                const dbrush_t *brush = &trace->bspdata->bspdata->dbrushes[brushidx];

                // only collide with objects you are interested in
                bool relevant_contents = ( brush->contents & trace->contents );
                if ( !relevant_contents )
                {
                        continue;
                }

                // only load SIMD if we have to
                if ( trace->bspdata->boxbrushes[brushidx].is_box && !simd_loaded )
                {
                        trace->load_simd();
                        simd_loaded = true;
                }   

                CM_ClipBoxToBrush<IS_POINT>( trace, brush, brushidx );
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

	float t1 = 0, t2 = 0;
	double offset = 0;
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
                node = trace->bspdata->bspdata->dnodes + num;
                const dplane_t *plane = trace->bspdata->bspdata->dplanes + node->planenum;
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
                        t1 = DotProduct( plane->normal, p1 ) - dist;
                        t2 = DotProduct( plane->normal, p2 ) - dist;
                        if ( IS_POINT )
                        {
                                offset = 0.0;
                        }
                        else
                        {
                                offset = DotProductAbsD( trace->extents, plane->normal );
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
void CM_BoxTrace( const Ray &ray, int headnode, int brushmask, bool compute_endpoint, const collbspdata_t *bspdata, Trace &trace )
{
        PStatTimer timer( bt_collector );

        trace.contents = brushmask;
        trace.start_pos = ray.start;
        trace.end_pos = ray.start + ray.delta;
        trace.extents = ray.extents;
        trace.delta = ray.delta;
        trace.inv_delta = ray.inv_delta();
        trace.mins = -ray.extents;
        trace.maxs = ray.extents;
        trace.is_point = ray.is_ray;
        trace.bspdata = (collbspdata_t *)bspdata;

        // general sweeping through the world
        CM_RecursiveHullCheck( &trace, headnode, 0, 1 );

        if ( compute_endpoint )
        {
                CM_ComputeTraceEndpoints( ray, &trace );
        }
}

collbspdata_t *SetupCollisionBSPData( const bspdata_t *bspdata )
{
        collbspdata_t *cdata = new collbspdata_t;
        memset( cdata, 0, sizeof( collbspdata_t ) );

        cdata->bspdata = bspdata;

        // find brushes with 6 sides, they are box brushes and we can accelerate the ray tracing
        for ( size_t brushnum = 0; brushnum < bspdata->dbrushes.size(); brushnum++ )
        {
                const dbrush_t *dbrush = &bspdata->dbrushes[brushnum];
                if ( dbrush->numsides == 6 )
                {
                        cboxbrush_t bbrush;
                        memset( &bbrush, 0, sizeof( cboxbrush_t ) );

                        int is_box = 1;

                        for ( int i = 0; i < 6; i++ )
                        {
                                const dbrushside_t *bside = &bspdata->dbrushsides[dbrush->firstside + i];
                                const dplane_t *plane = &bspdata->dplanes[bside->planenum];
                                const short t = bside->texinfo;
                                const planetypes axis = plane->type;

                                // only axis-aligned brushes are box brushes
                                if ( plane->type > plane_z )
                                {
                                        is_box = 0;
                                        break;
                                }

                                if ( plane->normal[axis] == 1.0 )
                                {
                                        bbrush.maxs[axis] = plane->dist;
                                        bbrush.surface_indices[axis + 3] = t;
                                }
                                else if ( plane->normal[axis] == -1.0 )
                                {
                                        bbrush.mins[axis] = -plane->dist;
                                        bbrush.surface_indices[axis] = t;
                                }

                                bbrush.ssemins = LoadAlignedSIMD( LVector4( bbrush.mins, 0 ).get_data() );
                                bbrush.ssemaxs = LoadAlignedSIMD( LVector4( bbrush.maxs, 0 ).get_data() );
                        }

                        if ( is_box )
                        {
                                bbrush.is_box = 1;
                                cdata->boxbrushes[brushnum] = bbrush;
                        }
                        
                }
        }

        return cdata;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// BSPTrace

BSPTrace::BSPTrace( BSPLoader *loader ) :
	_loader( loader )
{
	RayTrace::initialize();
	_scene = new RayTraceScene;
	_scene->set_build_quality( RayTraceScene::BUILD_QUALITY_HIGH );
}

void BSPTrace::add_dmodel( const dmodel_t *model, unsigned int mask )
{
	nassertv( _scene != nullptr );

	const bspdata_t *data = _loader->get_bspdata();

	for ( int facenum = 0; facenum < model->numfaces; facenum++ )
	{
		const dface_t *face = &data->dfaces[model->firstface + facenum];

		PT( RayTraceTriangleMesh ) geom = new RayTraceTriangleMesh;
		geom->set_build_quality( RayTraceScene::BUILD_QUALITY_HIGH );
		geom->set_mask( mask );

		int ntris = face->numedges - 2;
		for ( int tri = 0; tri < ntris; tri++ )
		{
			geom->add_triangle( VertCoord( data, face, 0 ),
				VertCoord( data, face, ( tri + 1 ) % face->numedges ),
				VertCoord( data, face, ( tri + 2 ) % face->numedges ) );
		}

		geom->build();

		_scene->add_geometry( geom );
		_geom_handles.push_back( geom );

		_dface_map[geom->get_geom_id()] = face;
	}

	_scene->update();
}

void BSPTrace::clear()
{
	_scene->remove_all();
	_dface_map.clear();
	_geom_handles.clear();
}
