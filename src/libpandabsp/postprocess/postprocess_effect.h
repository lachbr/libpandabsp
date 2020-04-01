/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file postprocess_effect.h
 * @author Brian Lach
 * @date July 24, 2019
 */

#ifndef POSTPROCESS_EFFECT_H
#define POSTPROCESS_EFFECT_H

#include "config_bsp.h"

#include <referenceCount.h>
#include <namable.h>
#include <simpleHashMap.h>
#include <pointerTo.h>

class PostProcessPass;
class PostProcess;
class GraphicsOutput;
class Texture;

class EXPCL_PANDABSP PostProcessEffect : public ReferenceCount, public Namable
{
	DECLARE_CLASS2( PostProcessEffect, ReferenceCount, Namable );

PUBLISHED:
	INLINE PostProcessEffect( PostProcess *pp, const std::string &name = "effect" ) :
		Namable( name ),
		_pp( pp )
	{
	}

	virtual Texture *get_final_texture() = 0;

	void add_pass( PostProcessPass *pass );
	void remove_pass( PostProcessPass *pass );
	PostProcessPass *get_pass( const std::string &name );

	virtual void setup();
	virtual void update();
	void window_event( GraphicsOutput *win );

	virtual void shutdown();

protected:
	PostProcess *_pp;
	SimpleHashMap<std::string, PT( PostProcessPass ), string_hash> _passes;
};

#endif // POSTPROCESS_EFFECT_H
