/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file static_props.cpp
 * @author Brian Lach
 * @date August 21, 2018
 */

#include "static_props.h"

IMPLEMENT_ATTRIB( StaticPropAttrib );

CPT( RenderAttrib ) StaticPropAttrib::make( bool static_lighting )
{
	StaticPropAttrib *attr = new StaticPropAttrib;
	attr->_static_lighting = static_lighting;
	return return_new( attr );
}

int StaticPropAttrib::compare_to_impl( const RenderAttrib *other ) const
{
	const StaticPropAttrib *spa_other = (const StaticPropAttrib *)other;
	if ( _static_lighting != spa_other->_static_lighting )
	{
		return _static_lighting < spa_other->_static_lighting ? -1 : 1;
	}
	return 0;
}

size_t StaticPropAttrib::get_hash_impl() const
{
	size_t hash = 0;
	hash = int_hash::add_hash( hash, (int)_static_lighting );
	return hash;
}