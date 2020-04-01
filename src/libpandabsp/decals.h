/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file decals.h
 * @author Brian Lach
 * @date March 08, 2019
 */

#ifndef BSP_DECALS_H
#define BSP_DECALS_H

#include "config_bsp.h"

#include <pdeque.h>
#include <pvector.h>
#include <nodePath.h>
#include <boundingBox.h>
#include <rigidBodyCombiner.h>

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
	int brush_modelnum;
};

class EXPCL_PANDABSP DecalManager
{
public:
	DecalManager( BSPLoader *loader );

	void init();

        // Trace a decal onto a brush model
	void decal_trace( const std::string &decal_material, const LPoint2 &decal_scale,
		float rotate, const LPoint3 &start, const LPoint3 &end,
		const LColorf &decal_color = LColorf( 1 ), const int flags = 0 );

	// Trace a decal onto a studio model
	void studio_decal_trace( const std::string &decal_material, const LPoint2 &decal_scale,
				 float rotate, const LPoint3 &start, const LPoint3 &end,
				 const LColorf &decal_color = LColorf( 1 ), const int flags = 0 );

        void cleanup();

	INLINE NodePath get_decal_root() const
	{
		return _decal_root;
	}

private:
	PT( RigidBodyCombiner ) _decal_rbc;
	NodePath _decal_root;
        BSPLoader *_loader;
        pdeque<PT( Decal )> _decals;
	pvector<PT( Decal )> _map_decals;
};

#endif // BSP_DECALS_H
