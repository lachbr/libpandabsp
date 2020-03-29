/**
 * PANDA3D BSP TOOLS
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file bsptools.h
 * @author Brian Lach
 * @date August 10, 2018
 *
 * @desc Tools/helpers shared by the compilers and libpandabsp.
 */

#ifndef BSPTOOLS_H
#define BSPTOOLS_H

#include "common_config.h"

#include <aa_luse.h>
#include "bspfile.h"
#include "mathlib/ssemath.h"

#define TEST_EPSILON 0.03125
#define DIST_EPSILON 0.03125

INLINE LVector3 angles_to_vector( const vec3_t &angles )
{
        return LVector3( cos( deg_2_rad( angles[0] ) ) * cos( deg_2_rad( angles[1] ) ),
                sin( deg_2_rad( angles[0] ) ) * cos( deg_2_rad( angles[1] ) ),
                sin( deg_2_rad( angles[1] ) ) );
}

struct _BSPEXPORT Ray
{
        Ray( const LPoint3 &s, const LPoint3 &e,
             const LPoint3 &mi, const LPoint3 &ma )
        {
                start = s;
                end = e;
                mins = mi;
                maxs = ma;
                delta = end - start;

                is_swept = delta.length_squared() != 0;
                extents = maxs - mins;
                extents *= 0.5;
                is_ray = extents.length_squared() < 1e-6;
                start_offset = ( mins + maxs ) / 2;
                start = ( start + start_offset );
                start_offset /= -1;
        }

        INLINE LVector3 inv_delta() const
        {
                LVector3 inv;
                for ( int i = 0; i < 3; i++ )
                {
                        if ( delta[i] != 0.0 )
                        {
                                inv[i] = 1.0 / delta[i];
                        }
                        else
                        {
                                inv[i] = FLT_MAX;
                        }
                }
                return inv;
        }

        LPoint3 start;
        LPoint3 end;
        LPoint3 mins;
        LPoint3 maxs;
        LVector3 extents;
        bool is_swept;
        bool is_ray;
        LVector3 start_offset;
        LVector3 delta;
};

struct collbspdata_t;

struct _BSPEXPORT Trace
{
        LPoint3 start_pos;
        LPoint3 end_pos;
        dplane_t plane;
        PN_stdfloat fraction;
        PN_stdfloat fraction_left_solid;
        int contents;
        int hit_contents;
        bool all_solid;
        bool start_solid;
        bool is_point;
        LPoint3 mins;
        LPoint3 maxs;
        LVector3 extents;
        LVector3 delta;
        LVector3 inv_delta;
        texinfo_t *surface;

        // filled in by load_simd()
        fltx4 sse_start;
        fltx4 sse_delta;
        fltx4 sse_inv_delta;
        fltx4 sse_extents;

        INLINE void load_simd()
        {
                sse_start = LoadAlignedSIMD( LVector4( start_pos, 0 ).get_data() );
                sse_extents = LoadAlignedSIMD( LVector4( extents, 0 ).get_data() );
                sse_delta = LoadAlignedSIMD( LVector4( delta, 0 ).get_data() );
                sse_inv_delta = LoadAlignedSIMD( LVector4( inv_delta, 0 ).get_data() );
        }

        collbspdata_t *bspdata;

        INLINE bool has_hit() const
        {
                return fraction != 1.0;
        }

        Trace() :
                fraction( 1.0 ),
                fraction_left_solid( 0.0 ),
                contents( 0 ),
                all_solid( false ),
                start_solid( false ),
                is_point( false ),
                surface( nullptr ),
                bspdata( nullptr ),
                hit_contents( CONTENTS_EMPTY )
        {
        }

};

class _BSPEXPORT BaseBSPEnumerator
{
public:
        BaseBSPEnumerator( bspdata_t *data );
        virtual bool enumerate_node( int node_id, const Ray &ray,
                                     float f, int context ) = 0;

        virtual bool enumerate_leaf( int leaf_id, const Ray &ray, float start,
                                     float end, int context ) = 0;

        virtual bool find_intersection( const Ray &ray ) = 0;

        bspdata_t *data;
};

extern _BSPEXPORT bool r_enumerate_nodes_along_ray( int node_id, const Ray &ray, float start,
                                         float end, BaseBSPEnumerator *surf, int context, float scale = 1.0 );

extern _BSPEXPORT bool enumerate_nodes_along_ray( const Ray &ray, BaseBSPEnumerator *surf, int context, float scale = 1.0 );

struct _BSPEXPORT lightfalloffparams_t
{
        float constant_atten;
        float linear_atten;
        float quadratic_atten;
        float radius;
        float start_fade_distance;
        float end_fade_distance;
        float cap_distance;
};
extern _BSPEXPORT lightfalloffparams_t GetLightFalloffParams( entity_t *e, LVector3 &intensity );

#endif // BSPTOOLS_H