#include "qrad.h"

#include <bsp_material.h>
#include <keyvalues.h>

#ifdef WORDS_BIGENDIAN
#error "HLRAD_TEXTURE doesn't support WORDS_BIGENDIAN, because I have no big endian machine to test it"
#endif

int g_numtextures;
radtexture_t *g_textures;

void DefaultTexture( radtexture_t *tex, const char *name )
{
        int i;
        PNMImage *img = new PNMImage( DEFAULT_LIGHTMAP_SIZE, DEFAULT_LIGHTMAP_SIZE );
        img->fill( 1.0 );
        tex->image = img;
        strcpy( tex->name, name );
        tex->name[MAX_TEXTURE_NAME - 1] = '\0';
        tex->bump = nullptr;
}

void LoadTextures()
{
        if ( !g_notextures )
        {
                Log( "Load Textures:\n" );
        }

        g_numtextures = g_bspdata->numtexrefs;
        g_textures = (radtexture_t *)malloc( g_numtextures * sizeof( radtexture_t ) );
        hlassume( g_textures != NULL, assume_NoMemory );
        int i;
        for ( i = 0; i < g_numtextures; i++ )
        {
                radtexture_t *tex = &g_textures[i];
                tex->bump = nullptr;
                tex->image = nullptr;

                bool b_explicit_reflectivity = false;
                LVector3 explicit_reflectivity;

                if ( g_notextures )
                {
                        DefaultTexture( tex, "DEFAULT" );
                }
                else
                {
                        texref_t *tref = &g_bspdata->dtexrefs[i];
                        string name = tref->name;
                        Filename fname = Filename::from_os_specific( name );
                        Log( "Loading material %s\n", fname.get_fullpath().c_str() );

                        const BSPMaterial *mat = BSPMaterial::get_from_file( fname );
                        if ( !mat )
                        {
                                Error( "Material %s not found!", fname.get_fullpath().c_str() );
                                continue;
                        }
                        if ( g_tex_contents.find( name ) == g_tex_contents.end() )
                        {
                                SetTextureContents( tref->name, mat->get_contents().c_str() );
                        }

                        if ( !mat->has_keyvalue( "$basetexture" ) )
                        {
                                Warning( "Material %s has no basetexture", fname.get_fullpath().c_str() );
                                continue;
                        }

                        if ( mat->has_keyvalue( "$reflectivity" ) )
                        {
                                b_explicit_reflectivity = true;
                                explicit_reflectivity = CKeyValues::to_3f( mat->get_keyvalue( "$reflectivity" ) );
                        }

                        PNMImage *img = new PNMImage;
                        if ( img->read( Filename::from_os_specific(
                                mat->get_keyvalue( "$basetexture" ) ) ) )
                        {
                                tex->image = img;
                                strcpy( tex->name, name.c_str() );
                                tex->name[MAX_TEXTURE_NAME - 1] = '\0';

                                PNMImage *bump = new PNMImage;
                                if ( mat->has_keyvalue( "$bumpmap" ) )
                                {
                                        Log( "Loaded RAD texture from %s and corresponding bump map %s.\n", tref->name,
                                             mat->get_keyvalue( "$bumpmap" ).c_str() );
                                        tex->bump = bump;
                                }
                                else
                                {
                                        Log( "Loaded RAD texture from %s.\n", tref->name );
                                        delete bump;
                                }
                                
                        }
                        else
                        {
                                Warning( "Could not load texture %s!\n", tref->name );
                                delete img;
                        }
                }

                if ( !b_explicit_reflectivity )
                {
                        vec3_t total;
                        VectorClear( total );
                        int width = tex->image->get_x_size();
                        int height = tex->image->get_y_size();
                        PNMImage *img = tex->image;
                        for ( int row = 0; row < height; row++ )
                        {
                                for ( int col = 0; col < width; col++ )
                                {
                                        vec3_t reflectivity;
                                        VectorCopy( img->get_xel( col, row ), reflectivity );
                                        for ( int k = 0; k < 3; k++ )
                                        {
                                                reflectivity[k] = pow( reflectivity[k], g_texreflectgamma );
                                        }
                                        VectorScale( reflectivity, g_texreflectscale, reflectivity );
                                        VectorAdd( total, reflectivity, total );
                                }
                        }
                        VectorScale( total, 1.0 / (double)( width * height ), total );
                        VectorCopy( total, tex->reflectivity );
                        Developer( DEVELOPER_LEVEL_MESSAGE, "Texture '%s': reflectivity is (%f,%f,%f).\n",
                                   tex->name, tex->reflectivity[0], tex->reflectivity[1], tex->reflectivity[2] );
                        if ( VectorMaximum( tex->reflectivity ) > 1.0 + NORMAL_EPSILON )
                        {
                                Warning( "Texture '%s': reflectivity (%f,%f,%f) greater than 1.0.", tex->name, tex->reflectivity[0], tex->reflectivity[1], tex->reflectivity[2] );
                        }
                }
                else
                {
                        explicit_reflectivity[0] = pow( explicit_reflectivity[0], g_texreflectgamma );
                        explicit_reflectivity[1] = pow( explicit_reflectivity[1], g_texreflectgamma );
                        explicit_reflectivity[2] = pow( explicit_reflectivity[2], g_texreflectgamma );
                        explicit_reflectivity *= g_texreflectscale;
                        VectorCopy( explicit_reflectivity, tex->reflectivity );
                }
        }
        if ( !g_notextures )
        {
                Log( "%i textures referenced\n", g_numtextures );
        }
}

