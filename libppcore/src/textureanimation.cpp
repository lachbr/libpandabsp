#include "textureanimation.h"

TypeDef( TextureAnimation );

TextureAnimation::TextureAnimation( NodePath &node, pvector<PT( Texture )> &texs, int fps ) :
        _node( node ),
        _textures( texs ),
        _fps( fps ),
        CInterval( "texanim", (double)texs.size() / (double)fps, false )
{
}

TextureAnimation::~TextureAnimation()
{
        _textures.clear();
        _node.clear();
        _fps = 0;
}

void TextureAnimation::priv_step( double t )
{
        CInterval::priv_step( t );
        int i = (int)( t * _fps ) % _textures.size();
        _node.set_texture( _textures[i], 1 );
}