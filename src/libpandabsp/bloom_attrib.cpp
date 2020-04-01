/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file bloom_attrib.cpp
 * @author Brian Lach
 * @date December 07, 2019
 */

#include "bloom_attrib.h"

IMPLEMENT_ATTRIB( BloomAttrib );

CPT( RenderAttrib ) BloomAttrib::make( bool bloom_enabled )
{
	BloomAttrib *attr = new BloomAttrib;
	attr->_bloom_enabled = bloom_enabled;

	return return_new( attr );
}

int BloomAttrib::compare_to_impl( const RenderAttrib *other ) const
{
	const BloomAttrib *ba = (const BloomAttrib *)other;

	if ( _bloom_enabled != ba->_bloom_enabled )
	{
		return _bloom_enabled < ba->_bloom_enabled ? -1 : 1;
	}

	return 0;
}

size_t BloomAttrib::get_hash_impl() const
{
	size_t hash = 0;
	hash = int_hash::add_hash( hash, (int)_bloom_enabled );

	return hash;
}