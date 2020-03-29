/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file cubemaps.h
 * @author Brian Lach
 * @date December 21, 2018
 */

#ifndef BSP_CUBEMAPS_H
#define BSP_CUBEMAPS_H

#include <aa_luse.h>
#include <texture.h>

class cubemap_t : public ReferenceCount
{
public:
        LVector3 pos;
        PT( Texture ) cubemap_tex;
        PNMImage cubemap_images[6];
        int leaf;
        int size;

        bool has_full_cubemap;
};

#endif // BSP_CUBEMAPS_H
