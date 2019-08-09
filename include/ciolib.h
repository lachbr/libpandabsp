/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file ciolib.h
 * @author Brian Lach
 * @date July 12, 2019
 */

#ifdef CIO

#ifndef CIOLIB_H
#define CIOLIB_H

#include <aa_luse.h>
#include <nodePath.h>

class CIOLib
{
PUBLISHED:
	static void set_pupil_direction( float x, float y, LVector3 &left, LVector3 &right );
	static LVector2 look_pupils_at( const NodePath &node, const LVector3 &point, const NodePath &eyes );
};

#endif // CIOLIB_H

#endif // CIO
