#include "csg.h"

#define MAXTEXNAME 256
#define MAX_TEXFILES 128

//  FindMiptex
//  TEX_InitFromWad
//  FindTexture
//  LoadLump
//  AddAnimatingTextures

// The old buggy code in effect limit the number of brush sides to MAX_MAP_BRUSHES
static char *texmap[MAX_INTERNAL_MAP_TEXINFO];
static int numtexmap = 0;

static int texmap_store( char *texname, bool shouldlock = true )
// This function should never be called unless a new entry in g_texinfo is being allocated.
{
        int i;
        if ( shouldlock )
        {
                ThreadLock();
        }
        hlassume( numtexmap < MAX_INTERNAL_MAP_TEXINFO, assume_MAX_MAP_TEXINFO ); // This error should never appear.

                                                                                  // Make sure we don't already have this textured stored.
        for ( i = 0; i < g_bspdata->numtexrefs; i++ )
        {
                texref_t *ref = &g_bspdata->dtexrefs[i];
                if ( strcmp( ref->name, texname ) == 0 )
                {
                        // This texture has already been referred to.
                        // Return the index of the already existing reference.
                        return i;
                }
        }

        safe_strncpy( g_bspdata->dtexrefs[g_bspdata->numtexrefs].name, texname, MAX_TEXTURE_NAME );
        g_bspdata->numtexrefs++;

        i = numtexmap;
        texmap[numtexmap] = texname;
        numtexmap++;

        if ( shouldlock )
        {
                ThreadUnlock();
        }
        return i;
}

static char *texmap_retrieve( int index )
{
        hlassume( 0 <= index && index < numtexmap, assume_first );
        return texmap[index];
}

static void texmap_clear()
{
        int i;
        ThreadLock();
        for ( i = 0; i < numtexmap; i++ )
        {
                free( texmap[i] );
        }
        numtexmap = 0;
        ThreadUnlock();
}

// =====================================================================================
//  CleanupName
// =====================================================================================
static void     CleanupName( const char* const in, char* out )
{
        int             i;

        for ( i = 0; i < MAXTEXNAME; i++ )
        {
                if ( !in[i] )
                {
                        break;
                }

                out[i] = toupper( in[i] );
        }

        for ( ; i < MAXTEXNAME; i++ )
        {
                out[i] = 0;
        }
}

// =====================================================================================
//  TEX_PandaInit
// =====================================================================================
bool TEX_Init()
{
        return false;
}

// =====================================================================================
//  TexinfoForBrushTexture
// =====================================================================================
int             TexinfoForBrushTexture( const plane_t* const plane, brush_texture_t* bt, const vec3_t origin
)
{
        vec3_t          vecs[2];
        int             sv, tv;
        vec_t           ang, sinv, cosv;
        vec_t           ns, nt;
        texinfo_t       tx;
        texinfo_t*      tc;
        int             i, j, k;

        if ( !strncasecmp( bt->name, "NULL", 4 ) )
        {
                return -1;
        }
        memset( &tx, 0, sizeof( tx ) );
        //FindMiptex (bt->name);

        contents_t contents = GetTextureContents( bt->name );

        // set the special flag
        if ( bt->name[0] == '*'
             || contents == CONTENTS_ORIGIN
             || contents == CONTENTS_NULL
             || !strncasecmp( bt->name, "aaatrigger", 10 )
             )
        {
                // actually only 'sky' and 'aaatrigger' needs this. --vluzacn
                tx.flags |= TEX_SPECIAL;
        }
        else if ( contents == CONTENTS_SKY )
        {
                tx.flags |= TEX_SPECIAL;
                tx.flags |= TEX_SKY;
                tx.flags |= TEX_SKY2D;
        }

        if ( bt->txcommand )
        {
                memcpy( tx.vecs, bt->vects.quark.vects, sizeof( tx.vecs ) );
                if ( origin[0] || origin[1] || origin[2] )
                {
                        tx.vecs[0][3] += DotProduct( origin, tx.vecs[0] );
                        tx.vecs[1][3] += DotProduct( origin, tx.vecs[1] );
                }
        }
        else
        {
                float shift_scale_u = 1 / 16.0;
                float shift_scale_v = 1 / 16.0;
                if ( g_nMapFileVersion < 220 )
                {
                        TextureAxisFromPlane( plane, vecs[0], vecs[1] );
                }

                if ( !bt->vects.valve.scale[0] )
                {
                        bt->vects.valve.scale[0] = 1;
                }
                if ( !bt->vects.valve.scale[1] )
                {
                        bt->vects.valve.scale[1] = 1;
                }

                if ( g_nMapFileVersion < 220 )
                {
                        // rotate axis
                        if ( bt->vects.valve.rotate == 0 )
                        {
                                sinv = 0;
                                cosv = 1;
                        }
                        else if ( bt->vects.valve.rotate == 90 )
                        {
                                sinv = 1;
                                cosv = 0;
                        }
                        else if ( bt->vects.valve.rotate == 180 )
                        {
                                sinv = 0;
                                cosv = -1;
                        }
                        else if ( bt->vects.valve.rotate == 270 )
                        {
                                sinv = -1;
                                cosv = 0;
                        }
                        else
                        {
                                ang = bt->vects.valve.rotate / 180 * Q_PI;
                                sinv = sin( ang );
                                cosv = cos( ang );
                        }

                        if ( vecs[0][0] )
                        {
                                sv = 0;
                        }
                        else if ( vecs[0][1] )
                        {
                                sv = 1;
                        }
                        else
                        {
                                sv = 2;
                        }

                        if ( vecs[1][0] )
                        {
                                tv = 0;
                        }
                        else if ( vecs[1][1] )
                        {
                                tv = 1;
                        }
                        else
                        {
                                tv = 2;
                        }

                        for ( i = 0; i < 2; i++ )
                        {
                                ns = cosv * vecs[i][sv] - sinv * vecs[i][tv];
                                nt = sinv * vecs[i][sv] + cosv * vecs[i][tv];
                                vecs[i][sv] = ns;
                                vecs[i][tv] = nt;
                        }

                        for ( i = 0; i < 2; i++ )
                        {
                                for ( j = 0; j < 3; j++ )
                                {
                                        tx.vecs[i][j] = vecs[i][j] / bt->vects.valve.scale[i];
                                }
                        }
                }
                else
                {
                        vec_t           scale;

                        scale = 1 / bt->vects.valve.scale[0];
                        VectorScale( bt->vects.valve.UAxis, scale, tx.vecs[0] );

                        scale = 1 / bt->vects.valve.scale[1];
                        VectorScale( bt->vects.valve.VAxis, scale, tx.vecs[1] );

                        scale = 1 / bt->vects.valve.lightmap_scale;
                        VectorScale( bt->vects.valve.UAxis, scale, tx.lightmap_vecs[0] );
                        VectorScale( bt->vects.valve.VAxis, scale, tx.lightmap_vecs[1] );

                        shift_scale_u = bt->vects.valve.scale[0] / bt->vects.valve.lightmap_scale;
                        shift_scale_v = bt->vects.valve.scale[1] / bt->vects.valve.lightmap_scale;
                }

                tx.vecs[0][3] = bt->vects.valve.shift[0] + DotProduct( origin, tx.vecs[0] );
                tx.vecs[1][3] = bt->vects.valve.shift[1] + DotProduct( origin, tx.vecs[1] );

                tx.lightmap_vecs[0][3] = shift_scale_u * bt->vects.valve.shift[0] + DotProduct( origin, tx.lightmap_vecs[0] );
                tx.lightmap_vecs[1][3] = shift_scale_v * bt->vects.valve.shift[1] + DotProduct( origin, tx.lightmap_vecs[1] );
                tx.lightmap_scale = bt->vects.valve.lightmap_scale;
        }

        //
        // find the g_texinfo
        //
        ThreadLock();
        tc = g_bspdata->texinfo;
        for ( i = 0; i < g_bspdata->numtexinfo; i++, tc++ )
        {
                // Sleazy hack 104, Pt 3 - Use strcmp on names to avoid dups
                if ( strcmp( texmap_retrieve( tc->texref ), bt->name ) != 0 )
                {
                        continue;
                }
                if ( tc->flags != tx.flags )
                {
                        continue;
                }
                for ( j = 0; j < 2; j++ )
                {
                        for ( k = 0; k < 4; k++ )
                        {
                                if ( tc->vecs[j][k] != tx.vecs[j][k] )
                                {
                                        goto skip;
                                }
                        }
                }
                ThreadUnlock();
                return i;
skip:;
        }

        hlassume( g_bspdata->numtexinfo < MAX_INTERNAL_MAP_TEXINFO, assume_MAX_MAP_TEXINFO );

        *tc = tx;
        tc->texref = texmap_store( bt->name, false );
        g_bspdata->numtexinfo++;
        ThreadUnlock();
        return i;
}

// Before WriteMiptex(), for each texinfo in g_texinfo, .texref is a string rather than texture index, so this function should be used instead of GetTextureByNumber.
const char *GetTextureByNumber_CSG( int texturenumber )
{
        if ( texturenumber == -1 )
                return "";
        return texmap_retrieve( g_bspdata->texinfo[texturenumber].texref );
}
