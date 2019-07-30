#include "texture_filter.h"

TypeDef( BSPTextureFilter );

PT( Texture ) BSPTextureFilter::post_load( Texture *tex )
{
	return tex;
}