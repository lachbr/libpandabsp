/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file lighting_origin_effect.cpp
 * @author Brian Lach
 * @date November 15, 2019
 */

#include "lighting_origin_effect.h"

IMPLEMENT_CLASS( LightingOriginEffect );

CPT( RenderEffect ) LightingOriginEffect::make( const LVector3 &origin )
{
	LightingOriginEffect *effect = new LightingOriginEffect;
	effect->_lighting_origin = origin;
	return return_new( effect );
}

bool LightingOriginEffect::safe_to_combine() const
{
	return true;
}

int LightingOriginEffect::compare_to_impl( const RenderEffect *other ) const
{
	const LightingOriginEffect *pother = (const LightingOriginEffect *)other;
	return _lighting_origin.compare_to( pother->_lighting_origin );
}

void LightingOriginEffect::register_with_read_factory()
{
	BamReader::get_factory()->register_factory( get_class_type(), make_from_bam );
}

void LightingOriginEffect::write_datagram( BamWriter *manager, Datagram &dg )
{
	RenderEffect::write_datagram( manager, dg );
	_lighting_origin.write_datagram( dg );
}

TypedWritable *LightingOriginEffect::make_from_bam( const FactoryParams &params )
{
	LightingOriginEffect *effect = new LightingOriginEffect;
	DatagramIterator scan;
	BamReader *manager;

	parse_params( params, scan, manager );
	effect->fillin( scan, manager );

	return effect;
}

void LightingOriginEffect::fillin( DatagramIterator &scan, BamReader *manager )
{
	RenderEffect::fillin( scan, manager );
	_lighting_origin.read_datagram( scan );
}