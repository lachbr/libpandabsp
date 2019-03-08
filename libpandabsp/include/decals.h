/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file decals.h
 * @author Brian Lach
 * @date March 08, 2019
 */

#ifndef BSP_DECALS_H
#define BSP_DECALS_H

#include "config_bsp.h"

#include <pdeque.h>
#include <nodePath.h>
#include <boundingBox.h>

class BSPLoader;

enum
{
        DECALFLAGS_NONE         = 0,
        DECALFLAGS_STATIC       = 1 << 0,
};

class Decal : public ReferenceCount
{
public:
        NodePath decalnp;
        PT( BoundingBox ) bounds;
        int flags;
};

class DecalManager
{
public:
        INLINE DecalManager( BSPLoader *loader ) :
                _loader( loader )
        {
        }

        // Trace a decal onto the world
        NodePath decal_trace( const std::string &decal_material, const LPoint2 &decal_scale,
                float rotate, const LPoint3 &start, const LPoint3 &end );

        void cleanup();

private:
        BSPLoader *_loader;
        pdeque<PT( Decal )> _decals;
};

#endif // BSP_DECALS_H