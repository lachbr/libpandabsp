#pragma once

#include "config_bsp.h"
#include <texturePoolFilter.h>

/**
 * Converts all textures to SRGB space when loaded.
 */
class BSPTextureFilter : public TexturePoolFilter
{
	TypeDecl( BSPTextureFilter, TexturePoolFilter );
public:
	virtual PT( Texture ) post_load( Texture *tex );
};