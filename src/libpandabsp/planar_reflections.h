/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file planar_reflections.h
 * @author Brian Lach
 * @date November 23, 2019
 */

#ifndef PLANAR_REFLECTIONS_H
#define PLANAR_REFLECTIONS_H

#include <nodePath.h>
#include <aa_luse.h>
#include <referenceCount.h>
#include <camera.h>
#include <graphicsOutput.h>
#include <planeNode.h>

#include "config_bsp.h"

class BSPShaderGenerator;
class Camera;
class GraphicsOutput;
class Texture;

class EXPCL_PANDABSP PlanarReflections : public ReferenceCount
{
public:
	PlanarReflections( BSPShaderGenerator *shgen );

	void setup( const LVector3 &planevec, float height );
	void shutdown();
	void update();

PUBLISHED:
	Texture *get_reflection_texture() const
	{
		return _reflection_texture;
	}

private:
	BSPShaderGenerator *_shgen;
	PT( Camera ) _camera;
	NodePath _camera_np;
	PT( Texture ) _reflection_texture;
	LPlane _plane;
	float _height;
	PT( GraphicsOutput ) _reflection_buffer;
	PT( PlaneNode ) _planenode;
	bool _is_setup;
};

#endif // PLANAR_REFLECTIONS_H