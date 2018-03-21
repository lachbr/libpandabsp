/**
* PANDA PLUS LIBRARY
* Copyright (c) 2017 Brian Lach <lachb@cat.pcsb.org>
*
* @file textureAnimation.h
* @author Brian Lach
* @date 2017-04-03
*/

#ifndef TEXTURE_ANIMATION_H
#define TEXTURE_ANIMATION_H

#include "config_pandaplus.h"
#include "pp_utils.h"

#include <pointerTo.h>
#include <texture.h>
#include <cInterval.h>
#include <nodePath.h>

class EXPCL_PANDAPLUS TextureAnimation : public CInterval
{

public:
        TextureAnimation( NodePath &node, pvector<PT( Texture )> &texs, int fps = 1 );
        virtual ~TextureAnimation();

        virtual void priv_step( double t );

private:
        NodePath _node;
        int _fps;
        pvector<PT( Texture )> _textures;

        TypeDecl( TextureAnimation, CInterval );

};

#endif // TEXTURE_ANIMATION_H