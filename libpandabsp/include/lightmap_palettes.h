#ifndef LIGHTMAP_PALETTES_H
#define LIGHTMAP_PALETTES_H

#include <pnmImage.h>
#include <pvector.h>
#include <notifyCategoryProxy.h>
#include <aa_luse.h>

#include "TexturePacker.h"
#include "mathlib.h"

class BSPLoader;
class TexturePacker;

struct LightmapPaletteDirectory
{
        struct LightmapPaletteEntry : public ReferenceCount
        {
                PT( Texture ) palette_tex;
        };

        struct LightmapFacePaletteEntry : public ReferenceCount
        {
                LightmapPaletteEntry *palette;
                int xshift, yshift;
                int palette_size[2];
                bool flipped;
        };

        pvector<PT(LightmapPaletteEntry)> entries;
        pvector<PT(LightmapFacePaletteEntry)> face_entries;
        pmap<int, LightmapFacePaletteEntry *> face_index;
};

struct LightmapSource
{
        int facenum;
        PNMImage lightmap_img;

        LightmapSource() :
                lightmap_img( 1, 1 )
        {

        }
};

struct Palette
{
        pvector<LightmapSource *> sources;
        PNMImage palette_img;
        TEXTURE_PACKER::TexturePacker *packer;
        Palette() :
                palette_img( 1, 1 )
        {
        }
};

NotifyCategoryDeclNoExport(lightmapPalettizer);

INLINE PN_stdfloat gamma_encode( PN_stdfloat linear, PN_stdfloat gamma )
{
        return pow( linear, 1.0 / gamma );
}

INLINE LRGBColor color_shift_pixel( colorrgbexp32_t *colsample, PN_stdfloat gamma )
{
        LVector3 sample( 0 );
        ColorRGBExp32ToVector( *colsample, sample );

        return LRGBColor( gamma_encode( sample[0] / 255.0, gamma ),
                          gamma_encode( sample[1] / 255.0, gamma ),
                          gamma_encode( sample[2] / 255.0, gamma ) );
}

class LightmapPalettizer
{
public:
        LightmapPalettizer( const BSPLoader *loader );
        LightmapPaletteDirectory palettize_lightmaps();

private:
        const BSPLoader *_loader;
        pvector<LightmapSource> _sources;
};

#endif // LIGHTMAP_PALETTES_H