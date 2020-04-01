/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file lighting_origin_effect.h
 * @author Brian Lach
 * @date November 15, 2019
 */

#ifndef LIGHTING_ORIGIN_EFFECT_H
#define LIGHTING_ORIGIN_EFFECT_H

#include "config_bsp.h"
#include <renderEffect.h>
#include <aa_luse.h>

class EXPCL_PANDABSP LightingOriginEffect : public RenderEffect
{
	DECLARE_CLASS( LightingOriginEffect, RenderEffect );

private:
	INLINE LightingOriginEffect() :
		RenderEffect(),
		_lighting_origin( 0, 0, 0 )
	{
	}

PUBLISHED:
	static CPT( RenderEffect ) make( const LVector3 &origin );

	INLINE LVector3 get_lighting_origin() const
	{
		return _lighting_origin;
	}

protected:
	virtual bool safe_to_combine() const;
	virtual int compare_to_impl( const RenderEffect *other ) const;

public:
	static void register_with_read_factory();
	virtual void write_datagram( BamWriter *manager, Datagram &dg );

protected:
	static TypedWritable *make_from_bam( const FactoryParams &params );
	void fillin( DatagramIterator &scan, BamReader *manager );

private:
	LVector3 _lighting_origin;
};

#endif // LIGHTING_ORIGIN_EFFECT_H