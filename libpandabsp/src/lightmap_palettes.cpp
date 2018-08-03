#include "lightmap_palettes.h"
#include "bspfile.h"
#include "bsploader.h"
#include "TexturePacker.h"

#include <bitset>
#include <pnmFileTypeJPG.h>
#include <cstdio>

NotifyCategoryDef( lightmapPalettizer, "" );

// Max size per palette before making a new one.
static const int max_palette = 1024;

LightmapPalettizer::LightmapPalettizer( const BSPLoader *loader ) :
        _loader( loader )
{
}

INLINE PNMImage lightmap_img_for_face( const BSPLoader *loader, const dface_t *face )
{
        int width = face->lightmap_size[0] + 1;
        int height = face->lightmap_size[1] + 1;
        int num_luxels = width * height;

        if ( num_luxels <= 0 )
        {
                lightmapPalettizer_cat.warning()
                        << "Face has 0 size lightmap, will appear fullbright" << std::endl;
                PNMImage img( 16, 16 );
                img.fill( 1.0 );
                return img;
        }

        PNMImage img( width, height );

        int luxel = 0;

        for ( int y = 0; y < height; y++ )
        {
                for ( int x = 0; x < width; x++ )
                {
                        LRGBColor luxel_col( 1, 1, 1 );

                        // To get the final pixel color, multiply all of the individual lightstyle samples together.
                        for ( int lightstyle = 0; lightstyle < 4; lightstyle++ )
                        {
                                if ( face->styles[lightstyle] == 0xFF )
                                {
                                        // Doesn't have this lightstyle.
                                        continue;
                                }

                                // From p3rad
                                int sample_idx = face->lightofs + lightstyle * num_luxels + luxel;
                                colorrgbexp32_t *sample = &g_dlightdata[sample_idx];

                                luxel_col.componentwise_mult( color_shift_pixel( sample, loader->get_gamma() ) );

                        }

                        img.set_xel( x, y, luxel_col );
                        luxel++;
                }
        }

        return img;
}

LightmapPaletteDirectory LightmapPalettizer::palettize_lightmaps()
{
        LightmapPaletteDirectory dir;

        pvector<Palette> result_vec;
        Palette pal;
        pal.packer = TEXTURE_PACKER::createTexturePacker();
        result_vec.push_back( pal );

        // First step, build sources.
        for ( int facenum = 0; facenum < g_numfaces; facenum++ )
        {
                dface_t *face = g_dfaces + facenum;
                if ( face->lightofs == -1 )
                {
                        // Face does not have a lightmap.
                        continue;
                }

                LightmapSource src;
                src.facenum = facenum;
                src.lightmap_img = lightmap_img_for_face( _loader, face );
                _sources.push_back( src );
        }

        for ( size_t i = 0; i < _sources.size(); i++ )
        {
                dface_t *face = g_dfaces + _sources[i].facenum;

                bool any_fit = false;
                // See if this lightmap can fit in any palette.
                for ( size_t j = 0; j < result_vec.size(); j++ )
                {
                        Palette *ppal = &result_vec[j];
                            
                        if ( ppal->packer->wouldTextureFit( face->lightmap_size[0] + 1, face->lightmap_size[1] + 1, true, false, max_palette, max_palette ) )
                        {
                                ppal->packer->addNewTexture( face->lightmap_size[0] + 1, face->lightmap_size[1] + 1 );
                                ppal->sources.push_back( &_sources[i] );
                                any_fit = true;
                                break;
                        }
                }

                if ( !any_fit )
                {
                        // We need to make a new palette for this lightmap, it won't fit in the current ones.
                        Palette newpal;
                        newpal.packer = TEXTURE_PACKER::createTexturePacker();
                        newpal.packer->addNewTexture( face->lightmap_size[0] + 1, face->lightmap_size[1] + 1 );
                        newpal.sources.push_back( &_sources[i] );
                        result_vec.push_back( newpal );
                }

        }

        // We've found a palette for each lightmap to fit in. Now we need to create the palette and remember
        // the offset into the palette for each face's lightmap.
        for ( size_t i = 0; i < result_vec.size(); i++ )
        {
                Palette *pal = &result_vec[i];
                int width, height;
                pal->packer->packTextures( width, height, true, false );
                pal->palette_img = PNMImage( width, height );
                pal->palette_img.fill( 0.0 );

                PT( LightmapPaletteDirectory::LightmapPaletteEntry ) entry = new LightmapPaletteDirectory::LightmapPaletteEntry;
                entry->palette_tex = new Texture;

                for ( size_t j = 0; j < pal->sources.size(); j++ )
                {
                        LightmapSource *src = pal->sources[j];
                        int xshift, yshift, lmwidth, lmheight;
                        bool rotated = pal->packer->getTextureLocation( j, xshift, yshift, lmwidth, lmheight );

                        PT( LightmapPaletteDirectory::LightmapFacePaletteEntry ) face_entry = new LightmapPaletteDirectory::LightmapFacePaletteEntry;
                        face_entry->palette = entry;
                        face_entry->flipped = rotated;
                        face_entry->xshift = xshift;
                        face_entry->yshift = yshift;
                        face_entry->palette_size[0] = width;
                        face_entry->palette_size[1] = height;
                        
                        for ( int y = 0; y < lmheight; y++ )
                        {
                                for ( int x = 0; x < lmwidth; x++ )
                                {
                                        pal->palette_img.set_xel( x + xshift, y + yshift, rotated ? src->lightmap_img.get_xel( y, x ) : src->lightmap_img.get_xel( x, y ) );
                                }
                        }

                        
                        dir.face_index[src->facenum] = face_entry;
                        dir.face_entries.push_back( face_entry );
                }

                entry->palette_tex->load( pal->palette_img );
                entry->palette_tex->set_magfilter( SamplerState::FT_linear );
                entry->palette_tex->set_minfilter( SamplerState::FT_linear_mipmap_linear );

                dir.entries.push_back( entry );

                TEXTURE_PACKER::releaseTexturePacker( pal->packer );
                pal->packer = nullptr;
        }

        return dir;
}