/**
 * PANDA3D BSP TOOLS
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file lightingutils.h
 * @author Brian Lach
 * @date July 31, 2018
 *
 */

#ifndef LIGHTINGUTILS_H
#define LIGHTINGUTILS_H

#include <aa_luse.h>

#include "bspfile.h"
#include "qrad.h"
#include "bsptools.h"
#include "lights.h"

extern float trace_leaf_brushes( int leaf_id, const LVector3 &start, const LVector3 &end, Trace &trace_out );
extern void clip_box_to_brush( Trace *trace, const LPoint3 &mins, const LPoint3 &maxs,
                               const LPoint3 &p1, const LPoint3 &p2, dbrush_t *brush );

extern void calc_ray_ambient_lighting( int thread, const LVector3 &start,
                                       const LVector3 &end, float tan_theta,
                                       LVector3 *color );
extern void calc_ray_ambient_lighting_SSE( int thread, const FourVectors &start,
                                           const FourVectors &end, fltx4 tan_theta,
                                           LVector3 color[4][MAX_LIGHTSTYLES] );

extern void compute_ambient_from_surface( dface_t *face, directlight_t *skylight,
                                          LRGBColor &color );
extern void compute_lightmap_color_from_average( dface_t *face, directlight_t *skylight,
                                                 float scale, LVector3 *colors );
extern void compute_lightmap_color_point_sample( dface_t *face, directlight_t *skylight, LTexCoordf &coord, float scale, LVector3 *colors );

extern void ComputeIndirectLightingAtPoint( const LVector3 &vpos, const LNormalf &vnormal, LVector3 &color, bool ignore_normals );
extern void ComputeDirectLightingAtPoint( const LVector3 &vpos, const LNormalf &vnormal, LVector3 &color );

#endif // LIGHTINGUTILS_H