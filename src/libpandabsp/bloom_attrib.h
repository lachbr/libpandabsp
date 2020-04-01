/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file bloom_attrib.h
 * @author Brian Lach
 * @date December 07, 2019
 */

#ifndef BLOOM_ATTRIB_H_
#define BLOOM_ATTRIB_H_

#include "config_bsp.h"

#include <renderAttrib.h>

class EXPCL_PANDABSP BloomAttrib : public RenderAttrib
{
	DECLARE_ATTRIB( BloomAttrib, RenderAttrib );

private:
	INLINE BloomAttrib() :
		RenderAttrib(),
		_bloom_enabled( true )
	{
	}

PUBLISHED:
	static CPT( RenderAttrib ) make( bool bloom_enabled = true );

	INLINE bool is_bloom_enabled() const
	{
		return _bloom_enabled;
	}

private:
	bool _bloom_enabled;
};

#endif // BLOOM_ATTRIB_H_