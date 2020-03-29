/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file postprocess_scene_pass.h
 * @author Brian Lach
 * @date July 24, 2019
 */

#ifndef POSTPROCESS_SCENE_PASS_H
#define POSTPROCESS_SCENE_PASS_H

#include "postprocess/postprocess_pass.h"

#include <renderState.h>

class PostProcess;

class PostProcessScenePass : public PostProcessPass
{
PUBLISHED:
	PostProcessScenePass( PostProcess *pp, int output_texture_bits = bits_PASSTEXTURE_COLOR, int auxbits = 0 );
	virtual void setup_region();
	virtual void setup_camera();
	virtual void setup_quad();
	virtual bool setup_buffer();

	void setup_scene_camera( int i );

	void set_camera_state( const RenderState *state );

private:
	int _aux_bits;
	CPT( RenderState ) _cam_state;
};

#endif // POSTPROCESS_SCENE_PASS_H
