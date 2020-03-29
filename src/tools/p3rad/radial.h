/**
 * PANDA3D BSP TOOLS
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file radial.h
 * @author Brian Lach
 * @date November 17, 2018
 *
 * @desc Lightmap filtering method.
 *       Derived from Source Engine's VRAD
 */

#ifndef RADIAL_H
#define RADIAL_H

#include "lightmap.h"

#define RADIAL_DIST_2   2
#define RADIAL_DIST     1.42

#define WEIGHT_EPS      0.00001

typedef struct radial_s
{
        int facenum;
        int w, h;
        float weight[MAX_SINGLEMAP];
        bumpsample_t light[MAX_SINGLEMAP];
} radial_t;

extern void FinalLightFace( const int facenum );
extern void WorldToLuxelSpace( lightinfo_t *l, const LVector3 &pos, LVector2 &coord );
extern void LuxelToWorldSpace( lightinfo_t *l, float s, float t, LVector3 &world );
extern void WorldToLuxelSpace( lightinfo_t *l, const FourVectors &world, FourVectors &coord );
extern void LuxelToWorldSpace( lightinfo_t *l, fltx4 s, fltx4 t, FourVectors &world );

#endif // RADIAL_H