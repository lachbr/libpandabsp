/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file static_props.h
 * @author Brian Lach
 * @date August 21, 2019
 */

#ifndef STATICPROPS_H
#define STATICPROPS_H

#include "config_bsp.h"

class EXPCL_PANDABSP StaticPropAttrib : RenderAttrib
{
	DECLARE_ATTRIB( StaticPropAttrib, RenderAttrib );

private:
	INLINE StaticPropAttrib() :
		RenderAttrib(),
		_static_lighting( false )
	{
	}

PUBLISHED:
	static CPT( RenderAttrib ) make( bool static_lighting = true );

	INLINE bool has_static_lighting() const
	{
		return _static_lighting;
	}

private:
	bool _static_lighting;
};

#endif // STATICPROPS_H