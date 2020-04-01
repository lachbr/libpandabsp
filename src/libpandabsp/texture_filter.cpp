#include "texture_filter.h"

IMPLEMENT_CLASS( BSPTextureFilter );

PT( Texture ) BSPTextureFilter::post_load( Texture *tex )
{
	return tex;
}