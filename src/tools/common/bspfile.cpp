#include "cmdlib.h"
#include "filelib.h"
#include "messages.h"
#include "hlassert.h"
#include "log.h"
#include "mathlib.h"
#include "bspfile.h"
#include "scriplib.h"
#include "blockmem.h"
#include <string>

//=============================================================================

int             g_max_map_texref = DEFAULT_MAX_MAP_TEXREF;
int				g_max_map_lightdata = DEFAULT_MAX_MAP_LIGHTDATA;

bspdata_t *g_bspdata = nullptr;

map<string, contents_t> g_tex_contents;

/*
* ===============
* FastChecksum
* ===============
*/

int FastChecksum( const void* const buffer, int bytes )
{
        int             checksum = 0;
        char*           buf = (char*)buffer;

        while ( bytes-- )
        {
                checksum = rotl( checksum, 4 ) ^ ( *buf );
                buf++;
        }

        return checksum;
}

/*
* ===============
* CompressVis
* ===============
*/
int             CompressVis( const byte* const src, const unsigned int src_length, byte* dest, unsigned int dest_length )
{
        unsigned int    j;
        byte*           dest_p = dest;
        unsigned int    current_length = 0;

        for ( j = 0; j < src_length; j++ )
        {
                current_length++;
                hlassume( current_length <= dest_length, assume_COMPRESSVIS_OVERFLOW );

                *dest_p = src[j];
                dest_p++;

                if ( src[j] )
                {
                        continue;
                }

                unsigned char   rep = 1;

                for ( j++; j < src_length; j++ )
                {
                        if ( src[j] || rep == 255 )
                        {
                                break;
                        }
                        else
                        {
                                rep++;
                        }
                }
                current_length++;
                hlassume( current_length <= dest_length, assume_COMPRESSVIS_OVERFLOW );

                *dest_p = rep;
                dest_p++;
                j--;
        }

        return dest_p - dest;
}

// =====================================================================================
//  DecompressVis
//      
// =====================================================================================
void            DecompressVis( bspdata_t *data, const byte* src, byte* const dest, const unsigned int dest_length )
{
        unsigned int    current_length = 0;
        int             c;
        byte*           out;
        int             row;

        row = ( data->dmodels[0].visleafs + 7 ) >> 3; // same as the length used by VIS program in CompressVis
                                                  // The wrong size will cause DecompressVis to spend extremely long time once the source pointer runs into the invalid area in g_dvisdata (for example, in BuildFaceLights, some faces could hang for a few seconds), and sometimes to crash.
        out = dest;

        do
        {
                hlassume( src - data->dvisdata < data->visdatasize, assume_DECOMPRESSVIS_OVERFLOW );
                if ( *src )
                {
                        current_length++;
                        hlassume( current_length <= dest_length, assume_DECOMPRESSVIS_OVERFLOW );

                        *out = *src;
                        out++;
                        src++;
                        continue;
                }

                hlassume( &src[1] - data->dvisdata < data->visdatasize, assume_DECOMPRESSVIS_OVERFLOW );
                c = src[1];
                src += 2;
                while ( c )
                {
                        current_length++;
                        hlassume( current_length <= dest_length, assume_DECOMPRESSVIS_OVERFLOW );

                        *out = 0;
                        out++;
                        c--;

                        if ( out - dest >= row )
                        {
                                return;
                        }
                }
        } while ( out - dest < row );
}

//
// =====================================================================================
//

// =====================================================================================
//  SwapBSPFile
//      byte swaps all data in a bsp file
// =====================================================================================
static void     SwapBSPFile( bspdata_t *data, const bool todisk )
{
        int             i, j;
        dmodel_t*       d;

        // models       
        for ( i = 0; i < data->nummodels; i++ )
        {
                d = &data->dmodels[i];

                for ( j = 0; j < MAX_MAP_HULLS; j++ )
                {
                        d->headnode[j] = LittleLong( d->headnode[j] );
                }

                d->visleafs = LittleLong( d->visleafs );
                d->firstface = LittleLong( d->firstface );
                d->numfaces = LittleLong( d->numfaces );

                for ( j = 0; j < 3; j++ )
                {
                        d->mins[j] = LittleFloat( d->mins[j] );
                        d->maxs[j] = LittleFloat( d->maxs[j] );
                        d->origin[j] = LittleFloat( d->origin[j] );
                }
        }

        //
        // vertexes
        //
        for ( i = 0; i < data->numvertexes; i++ )
        {
                for ( j = 0; j < 3; j++ )
                {
                        data->dvertexes[i].point[j] = LittleFloat( data->dvertexes[i].point[j] );
                }
        }

        //
        // planes
        //      
        for ( i = 0; i < data->numplanes; i++ )
        {
                for ( j = 0; j < 3; j++ )
                {
                        data->dplanes[i].normal[j] = LittleFloat( data->dplanes[i].normal[j] );
                }
                data->dplanes[i].dist = LittleFloat( data->dplanes[i].dist );
                data->dplanes[i].type = (planetypes)LittleLong( data->dplanes[i].type );
        }

        //
        // texinfos
        //      
        for ( i = 0; i < data->numtexinfo; i++ )
        {
                for ( j = 0; j < 8; j++ )
                {
                        data->texinfo[i].vecs[0][j] = LittleFloat( data->texinfo[i].vecs[0][j] );
                }
                data->texinfo[i].texref = LittleLong( data->texinfo[i].texref );
                data->texinfo[i].flags = LittleLong( data->texinfo[i].flags );
        }

        //
        // faces
        //
        for ( i = 0; i < data->numfaces; i++ )
        {
                data->dfaces[i].texinfo = LittleShort( data->dfaces[i].texinfo );
                data->dfaces[i].planenum = LittleShort( data->dfaces[i].planenum );
                data->dfaces[i].side = LittleShort( data->dfaces[i].side );
                data->dfaces[i].lightofs = LittleLong( data->dfaces[i].lightofs );
                data->dfaces[i].firstedge = LittleLong( data->dfaces[i].firstedge );
                data->dfaces[i].numedges = LittleShort( data->dfaces[i].numedges );
        }

        /*
        //
        // orig faces
        //
        for ( i = 0; i < g_numorigfaces; i++ )
        {
        g_dorigfaces[i].texinfo = LittleShort( g_dorigfaces[i].texinfo );
        g_dorigfaces[i].planenum = LittleShort( g_dorigfaces[i].planenum );
        g_dorigfaces[i].side = LittleShort( g_dorigfaces[i].side );
        g_dorigfaces[i].lightofs = LittleLong( g_dorigfaces[i].lightofs );
        g_dorigfaces[i].firstedge = LittleLong( g_dorigfaces[i].firstedge );
        g_dorigfaces[i].numedges = LittleShort( g_dorigfaces[i].numedges );
        }
        */

        //
        // nodes
        //
        for ( i = 0; i < data->numnodes; i++ )
        {
                data->dnodes[i].planenum = LittleLong( data->dnodes[i].planenum );
                for ( j = 0; j < 3; j++ )
                {
                        data->dnodes[i].mins[j] = LittleShort( data->dnodes[i].mins[j] );
                        data->dnodes[i].maxs[j] = LittleShort( data->dnodes[i].maxs[j] );
                }
                data->dnodes[i].children[0] = LittleShort( data->dnodes[i].children[0] );
                data->dnodes[i].children[1] = LittleShort( data->dnodes[i].children[1] );
                data->dnodes[i].firstface = LittleShort( data->dnodes[i].firstface );
                data->dnodes[i].numfaces = LittleShort( data->dnodes[i].numfaces );
        }

        //
        // leafs
        //
        for ( i = 0; i < data->numleafs; i++ )
        {
                data->dleafs[i].contents = LittleLong( data->dleafs[i].contents );
                for ( j = 0; j < 3; j++ )
                {
                        data->dleafs[i].mins[j] = LittleShort( data->dleafs[i].mins[j] );
                        data->dleafs[i].maxs[j] = LittleShort( data->dleafs[i].maxs[j] );
                }

                data->dleafs[i].firstmarksurface = LittleShort( data->dleafs[i].firstmarksurface );
                data->dleafs[i].nummarksurfaces = LittleShort( data->dleafs[i].nummarksurfaces );
                data->dleafs[i].visofs = LittleLong( data->dleafs[i].visofs );
        }

        //
        // texrefs
        //
        for ( i = 0; i < data->numtexrefs; i++ )
        {
                for ( int j = 0; j < MAX_TEXTURE_NAME; j++ )
                {
                        data->dtexrefs[i].name[j] = LittleShort( data->dtexrefs[i].name[j] );
                }
        }

        //
        // marksurfaces
        //
        for ( i = 0; i < data->nummarksurfaces; i++ )
        {
                data->dmarksurfaces[i] = LittleShort( data->dmarksurfaces[i] );
        }

        //
        // surfedges
        //
        for ( i = 0; i < data->numsurfedges; i++ )
        {
                data->dsurfedges[i] = LittleLong( data->dsurfedges[i] );
        }

        //
        // edges
        //
        for ( i = 0; i < data->numedges; i++ )
        {
                data->dedges[i].v[0] = LittleShort( data->dedges[i].v[0] );
                data->dedges[i].v[1] = LittleShort( data->dedges[i].v[1] );
        }

        // leaf ambient index
        for ( i = 0; i < (int)data->leafambientindex.size(); i++ )
        {
                dleafambientindex_t *idx = &data->leafambientindex[i];
                idx->first_ambient_sample = LittleShort( idx->first_ambient_sample );
                idx->num_ambient_samples = LittleShort( idx->num_ambient_samples );
        }

        // brush
        for ( i = 0; i < (int)data->dbrushes.size(); i++ )
        {
                data->dbrushes[i].firstside = LittleLong( data->dbrushes[i].firstside );
                data->dbrushes[i].numsides = LittleLong( data->dbrushes[i].numsides );
                data->dbrushes[i].contents = LittleLong( data->dbrushes[i].contents );
        }

        // brush sides
        for ( i = 0; i < (int)data->dbrushsides.size(); i++ )
        {
                dbrushside_t *side = &data->dbrushsides[i];
                side->bevel = LittleShort( side->bevel );
                side->planenum = LittleShort( side->planenum );
                side->texinfo = LittleShort( side->texinfo );
        }

        // leaf brushes
        for ( i = 0; i < (int)data->dleafbrushes.size(); i++ )
        {
                data->dleafbrushes[i] = LittleShort( data->dleafbrushes[i] );
        }
}

// =====================================================================================
//  CopyLump
//      balh
// =====================================================================================
static int      CopyLump( int lump, void* dest, int size, const dheader_t* const header )
{
        int             length, ofs;

        length = header->lumps[lump].filelen;
        ofs = header->lumps[lump].fileofs;

        if ( length % size )
        {
                Error( "LoadBSPFile: odd lump size for lump %i, length %i, size %i", lump, length, size );
        }

        //special handling for tex and lightdata to keep things from exploding - KGP
        //if ( lump == LUMP_TEXTURES && dest == (void*)g_dtexrefs )
        //{
        //        hlassume( g_max_map_texref > length, assume_MAX_MAP_MIPTEX );
        //}

        memcpy( dest, (byte*)header + ofs, length );

        return length / size;
}

template<class T>
static int CopyLump( int lump, pvector<T> &dest, const dheader_t* const header )
{
        dest.resize( header->lumps[lump].filelen / sizeof( T ) );
        return CopyLump( lump, dest.data(), sizeof( T ), header );
}


// =====================================================================================
//  LoadBSPFile
//      balh
// =====================================================================================
bspdata_t            *LoadBSPFile( const char* const filename )
{
        dheader_t* header;
        LoadFile( filename, (char**)&header );
        return LoadBSPImage( header );
}

// =====================================================================================
//  LoadBSPImage
//      balh
// =====================================================================================
bspdata_t            *LoadBSPImage( dheader_t* const header )
{
        unsigned int     i;

        // swap the header
        for ( i = 0; i < sizeof( dheader_t ) / 4; i++ )
        {
                ( (int*)header )[i] = LittleLong( ( (int*)header )[i] );
        }

        if ( header->ident != PBSP_MAGIC )
        {
                Error( "Not a valid PBSP file. Ident of file is %i, not %i", header->ident, PBSP_MAGIC );
        }

        if ( header->version != BSPVERSION )
        {
                Error( "BSP is version %i, not %i", header->version, BSPVERSION );
        }

        bspdata_t *data = new bspdata_t;

        data->nummodels = CopyLump( LUMP_MODELS, data->dmodels, sizeof( dmodel_t ), header );
        data->numvertexes = CopyLump( LUMP_VERTEXES, data->dvertexes, sizeof( dvertex_t ), header );
        data->numplanes = CopyLump( LUMP_PLANES, data->dplanes, sizeof( dplane_t ), header );
        data->numleafs = CopyLump( LUMP_LEAFS, data->dleafs, sizeof( dleaf_t ), header );
        data->numnodes = CopyLump( LUMP_NODES, data->dnodes, sizeof( dnode_t ), header );
        data->numtexinfo = CopyLump( LUMP_TEXINFO, data->texinfo, sizeof( texinfo_t ), header );
        data->numfaces = CopyLump( LUMP_FACES, data->dfaces, sizeof( dface_t ), header );
        //data->numorigfaces = CopyLump( LUMP_ORIGFACES, data->dorigfaces, sizeof( dface_t ), header );
        data->nummarksurfaces = CopyLump( LUMP_MARKSURFACES, data->dmarksurfaces, sizeof( data->dmarksurfaces[0] ), header );
        data->numsurfedges = CopyLump( LUMP_SURFEDGES, data->dsurfedges, sizeof( data->dsurfedges[0] ), header );
        data->numedges = CopyLump( LUMP_EDGES, data->dedges, sizeof( dedge_t ), header );
        data->numtexrefs = CopyLump( LUMP_TEXTURES, data->dtexrefs, sizeof( texref_t ), header );
        data->visdatasize = CopyLump( LUMP_VISIBILITY, data->dvisdata, 1, header );
        data->entdatasize = CopyLump( LUMP_ENTITIES, data->dentdata, 1, header );

        // new lumps uses STL vectors and templates!
        CopyLump( LUMP_BRUSHES, data->dbrushes, header );
        CopyLump( LUMP_BRUSHSIDES, data->dbrushsides, header );
        CopyLump( LUMP_LEAFBRUSHES, data->dleafbrushes, header );
        CopyLump( LUMP_LEAFAMBIENTINDEX, data->leafambientindex, header );
        CopyLump( LUMP_LEAFAMBIENTLIGHTING, data->leafambientlighting, header );
	CopyLump( LUMP_BOUNCEDLIGHTING, data->bouncedlightdata, header );
        CopyLump( LUMP_DIRECTLIGHTING, data->lightdata, header );
	CopyLump( LUMP_DIRECTSUNLIGHTING, data->sunlightdata, header );
        CopyLump( LUMP_STATICPROPS, data->dstaticprops, header );
        CopyLump( LUMP_STATICPROPVERTEXDATA, data->dstaticpropvertexdatas, header );
        CopyLump( LUMP_STATICPROPLIGHTING, data->staticproplighting, header );
        CopyLump( LUMP_VERTNORMALS, data->vertnormals, header );
        CopyLump( LUMP_VERTNORMALINDICES, data->vertnormalindices, header );
        CopyLump( LUMP_CUBEMAPDATA, data->cubemapdata, header );
        CopyLump( LUMP_CUBEMAPS, data->cubemaps, header );

        Free( header );                                          // everything has been copied out

                                                                 //
                                                                 // swap everything
                                                                 //      
        SwapBSPFile( data, false );

        data->dmodels_checksum = FastChecksum( data->dmodels, data->nummodels * sizeof( data->dmodels[0] ) );
        data->dvertexes_checksum = FastChecksum( data->dvertexes, data->numvertexes * sizeof( data->dvertexes[0] ) );
        data->dplanes_checksum = FastChecksum( data->dplanes, data->numplanes * sizeof( data->dplanes[0] ) );
        data->dleafs_checksum = FastChecksum( data->dleafs, data->numleafs * sizeof( data->dleafs[0] ) );
        data->dnodes_checksum = FastChecksum( data->dnodes, data->numnodes * sizeof( data->dnodes[0] ) );
        data->texinfo_checksum = FastChecksum( data->texinfo, data->numtexinfo * sizeof( data->texinfo[0] ) );
        data->dfaces_checksum = FastChecksum( data->dfaces, data->numfaces * sizeof( data->dfaces[0] ) );
        //data->dorigfaces_checksum = FastChecksum( data->dorigfaces, data->numorigfaces * sizeof( data->dorigfaces[0] ) );
        data->dmarksurfaces_checksum = FastChecksum( data->dmarksurfaces, data->nummarksurfaces * sizeof( data->dmarksurfaces[0] ) );
        data->dsurfedges_checksum = FastChecksum( data->dsurfedges, data->numsurfedges * sizeof( data->dsurfedges[0] ) );
        data->dedges_checksum = FastChecksum( data->dedges, data->numedges * sizeof( data->dedges[0] ) );
        data->dtexrefs_checksum = FastChecksum( data->dtexrefs, data->numedges * sizeof( data->dtexrefs[0] ) );
        data->dvisdata_checksum = FastChecksum( data->dvisdata, data->visdatasize * sizeof( data->dvisdata[0] ) );
        data->dlightdata_checksum = FastChecksum( data->lightdata.data(), data->lightdata.size() * sizeof( colorrgbexp32_t ) );
        data->dentdata_checksum = FastChecksum( data->dentdata, data->entdatasize * sizeof( data->dentdata[0] ) );

        return data;
}

//
// =====================================================================================
//

// =====================================================================================
//  AddLump
//      balh
// =====================================================================================
static void     AddLump( int lumpnum, void* data, int len, dheader_t* header, FILE* bspfile )
{
        lump_t* lump = &header->lumps[lumpnum];
        lump->fileofs = LittleLong( ftell( bspfile ) );
        lump->filelen = LittleLong( len );
        SafeWrite( bspfile, data, ( len + 3 ) & ~3 );
}

template<class T>
static void AddLump( int lumpnum, pvector<T> &data, dheader_t *header, FILE *bspfile )
{
        AddLump( lumpnum, data.data(), data.size() * sizeof( T ), header, bspfile );
}

// =====================================================================================
//  WriteBSPFile
//      Swaps the bsp file in place, so it should not be referenced again
// =====================================================================================
void            WriteBSPFile( bspdata_t *data, const char* const filename )
{
        dheader_t       outheader;
        dheader_t*      header;
        FILE*           bspfile;

        header = &outheader;
        memset( header, 0, sizeof( dheader_t ) );

        SwapBSPFile( data, true );

        header->ident = LittleLong( PBSP_MAGIC );
        header->version = LittleLong( BSPVERSION );

        bspfile = SafeOpenWrite( filename );
        SafeWrite( bspfile, header, sizeof( dheader_t ) );         // overwritten later

                                                                   //      LUMP TYPE       DATA            LENGTH                              HEADER  BSPFILE   
        AddLump( LUMP_PLANES, data->dplanes, data->numplanes * sizeof( dplane_t ), header, bspfile );
        AddLump( LUMP_LEAFS, data->dleafs, data->numleafs * sizeof( dleaf_t ), header, bspfile );
        AddLump( LUMP_VERTEXES, data->dvertexes, data->numvertexes * sizeof( dvertex_t ), header, bspfile );
        AddLump( LUMP_NODES, data->dnodes, data->numnodes * sizeof( dnode_t ), header, bspfile );
        AddLump( LUMP_TEXINFO, data->texinfo, data->numtexinfo * sizeof( texinfo_t ), header, bspfile );
        AddLump( LUMP_FACES, data->dfaces, data->numfaces * sizeof( dface_t ), header, bspfile );
        //AddLump( LUMP_ORIGFACES, data->dorigfaces, data->numorigfaces * sizeof( dface_t ), header, bspfile );

        AddLump( LUMP_MARKSURFACES, data->dmarksurfaces, data->nummarksurfaces * sizeof( data->dmarksurfaces[0] ), header, bspfile );
        AddLump( LUMP_SURFEDGES, data->dsurfedges, data->numsurfedges * sizeof( data->dsurfedges[0] ), header, bspfile );
        AddLump( LUMP_EDGES, data->dedges, data->numedges * sizeof( dedge_t ), header, bspfile );
        AddLump( LUMP_MODELS, data->dmodels, data->nummodels * sizeof( dmodel_t ), header, bspfile );
        AddLump( LUMP_TEXTURES, data->dtexrefs, data->numtexrefs * sizeof( texref_t ), header, bspfile );
        AddLump( LUMP_VISIBILITY, data->dvisdata, data->visdatasize, header, bspfile );
        AddLump( LUMP_ENTITIES, data->dentdata, data->entdatasize, header, bspfile );

        // new lumps uses STL vectors and templates!
        AddLump( LUMP_BRUSHES, data->dbrushes, header, bspfile );
        AddLump( LUMP_BRUSHSIDES, data->dbrushsides, header, bspfile );
        AddLump( LUMP_LEAFBRUSHES, data->dleafbrushes, header, bspfile );
        AddLump( LUMP_LEAFAMBIENTINDEX, data->leafambientindex, header, bspfile );
        AddLump( LUMP_LEAFAMBIENTLIGHTING, data->leafambientlighting, header, bspfile );
	AddLump( LUMP_BOUNCEDLIGHTING, data->bouncedlightdata, header, bspfile );
        AddLump( LUMP_DIRECTLIGHTING, data->lightdata, header, bspfile );
	AddLump( LUMP_DIRECTSUNLIGHTING, data->sunlightdata, header, bspfile );
        AddLump( LUMP_STATICPROPS, data->dstaticprops, header, bspfile );
        AddLump( LUMP_STATICPROPVERTEXDATA, data->dstaticpropvertexdatas, header, bspfile );
        AddLump( LUMP_STATICPROPLIGHTING, data->staticproplighting, header, bspfile );
        AddLump( LUMP_VERTNORMALS, data->vertnormals, header, bspfile );
        AddLump( LUMP_VERTNORMALINDICES, data->vertnormalindices, header, bspfile );
        AddLump( LUMP_CUBEMAPDATA, data->cubemapdata, header, bspfile );
        AddLump( LUMP_CUBEMAPS, data->cubemaps, header, bspfile );

        fseek( bspfile, 0, SEEK_SET );
        SafeWrite( bspfile, header, sizeof( dheader_t ) );

        fclose( bspfile );
}


#ifdef PLATFORM_CAN_CALC_EXTENT
// =====================================================================================
//  GetFaceExtents (with PLATFORM_CAN_CALC_EXTENT on)
// =====================================================================================
#ifdef _WIN32
#ifdef VERSION_32BIT
static void CorrectFPUPrecision()
{
        unsigned int currentcontrol;
        if ( _controlfp_s( &currentcontrol, 0, 0 ) )
        {
                Warning( "Couldn't get FPU precision" );
        }
        else
        {
                unsigned int val = ( currentcontrol & _MCW_PC );
                if ( val != _PC_53 )
                {
                        Warning( "FPU precision is %s. Setting to %s.", ( val == _PC_24 ? "24" : val == _PC_64 ? "64" : "invalid" ), "53" );
                        if ( _controlfp_s( &currentcontrol, _PC_53, _MCW_PC )
                             || ( currentcontrol & _MCW_PC ) != _PC_53 )
                        {
                                Warning( "Couldn't set FPU precision" );
                        }
                }
        }
}
#endif
#ifdef VERSION_64BIT
static void CorrectFPUPrecision()
{
        // do nothing, because we use SSE registers
}
#endif
#endif

#ifdef SYSTEM_POSIX
static void CorrectFPUPrecision()
{
        // just leave it to default and see if CalcFaceExtents_test gives us any error
}
#endif

float CalculatePointVecsProduct( const volatile float *point, const volatile float *vecs )
{
        volatile double val;
        volatile double tmp;

        val = (double)point[0] * (double)vecs[0]; // always do one operation at a time and save to memory
        tmp = (double)point[1] * (double)vecs[1];
        val = val + tmp;
        tmp = (double)point[2] * (double)vecs[2];
        val = val + tmp;
        val = val + (double)vecs[3];

        return (float)val;
}

bool CalcFaceExtents_test()
{
        const int numtestcases = 6;
        volatile float testcases[numtestcases][8] = {
                { 1, 1, 1, 1, 0.375 * DBL_EPSILON, 0.375 * DBL_EPSILON, -1, 0 },
        { 1, 1, 1, 0.375 * DBL_EPSILON, 0.375 * DBL_EPSILON, 1, -1, DBL_EPSILON },
        { DBL_EPSILON, DBL_EPSILON, 1, 0.375, 0.375, 1, -1, DBL_EPSILON },
        { 1, 1, 1, 1, 1, 0.375 * FLT_EPSILON, -2, 0.375 * FLT_EPSILON },
        { 1, 1, 1, 1, 0.375 * FLT_EPSILON, 1, -2, 0.375 * FLT_EPSILON },
        { 1, 1, 1, 0.375 * FLT_EPSILON, 1, 1, -2, 0.375 * FLT_EPSILON } };
        bool ok;

        // If the test failed, please check:
        //   1. whether the calculation is performed on FPU
        //   2. whether the register precision is too low

        CorrectFPUPrecision();

        ok = true;
        for ( int i = 0; i < 6; i++ )
        {
                float val = CalculatePointVecsProduct( &testcases[i][0], &testcases[i][3] );
                if ( val != testcases[i][7] )
                {
                        Warning( "internal error: CalcFaceExtents_test failed on case %d (%.20f != %.20f).", i, val, testcases[i][7] );
                        ok = false;
                }
        }
        return ok;
}

void GetFaceExtents( bspdata_t *data, int facenum, int mins_out[2], int maxs_out[2] )
{
        CorrectFPUPrecision();

        dface_t *f;
        float mins[2], maxs[2], val;
        int i, j, e;
        dvertex_t *v;
        texinfo_t *tex;
        int bmins[2], bmaxs[2];

        f = &data->dfaces[facenum];

        mins[0] = mins[1] = 999999;
        maxs[0] = maxs[1] = -99999;

        tex = &data->texinfo[f->texinfo];

        for ( i = 0; i < f->numedges; i++ )
        {
                e = data->dsurfedges[f->firstedge + i];
                if ( e >= 0 )
                {
                        v = &data->dvertexes[data->dedges[e].v[0]];
                }
                else
                {
                        v = &data->dvertexes[data->dedges[-e].v[1]];
                }
                for ( j = 0; j < 2; j++ )
                {
                        // The old code: val = v->point[0] * tex->vecs[j][0] + v->point[1] * tex->vecs[j][1] + v->point[2] * tex->vecs[j][2] + tex->vecs[j][3];
                        //   was meant to be compiled for x86 under MSVC (prior to VS 11), so the intermediate values were stored as 64-bit double by default.
                        // The new code will produce the same result as the old code, but it's portable for different platforms.
                        // See this article for details: Intermediate Floating-Point Precision by Bruce-Dawson http://www.altdevblogaday.com/2012/03/22/intermediate-floating-point-precision/

                        // The essential reason for having this ugly code is to get exactly the same value as the counterpart of game engine.
                        // The counterpart of game engine is the function CalcFaceExtents in HLSDK.
                        // So we must also know how Valve compiles HLSDK. I think Valve compiles HLSDK with VC6.0 in the past.
                        val = CalculatePointVecsProduct( v->point, tex->lightmap_vecs[j] );
                        if ( val < mins[j] )
                        {
                                mins[j] = val;
                        }
                        if ( val > maxs[j] )
                        {
                                maxs[j] = val;
                        }
                }
        }

        for ( i = 0; i < 2; i++ )
        {
                bmins[i] = (int)floor( mins[i] );
                bmaxs[i] = (int)ceil( maxs[i] );
        }

        for ( i = 0; i < 2; i++ )
        {
                mins_out[i] = bmins[i];
                maxs_out[i] = bmaxs[i];
        }
}

// =====================================================================================
//  WriteExtentFile
// =====================================================================================
void WriteExtentFile( bspdata_t *data, const char *const filename )
{
        FILE *f;
        f = fopen( filename, "w" );
        if ( !f )
        {
                Error( "Error opening %s: %s", filename, strerror( errno ) );
        }
        fprintf( f, "%i\n", data->numfaces );
        for ( int i = 0; i < data->numfaces; i++ )
        {
                int mins[2];
                int maxs[2];
                GetFaceExtents( data, i, mins, maxs );
                fprintf( f, "%i %i %i %i\n", mins[0], mins[1], maxs[0], maxs[1] );
        }
        fclose( f );
}

#else

typedef struct
{
        int mins[2];
        int maxs[2];
}
faceextent_t;

bool g_faceextents_loaded = false;
faceextent_t g_faceextents[MAX_MAP_FACES]; //[g_numfaces]

                                           // =====================================================================================
                                           //  LoadExtentFile
                                           // =====================================================================================
void LoadExtentFile( const char *const filename )
{
        FILE *f;
        f = fopen( filename, "r" );
        if ( !f )
        {
                Error( "Error opening %s: %s", filename, strerror( errno ) );
        }
        int count;
        int numfaces;
        count = fscanf( f, "%i\n", (int *)&numfaces );
        if ( count != 1 )
        {
                Error( "LoadExtentFile (line %i): scanf failure", 1 );
        }
        if ( numfaces != g_numfaces )
        {
                Error( "LoadExtentFile: numfaces(%i) doesn't match g_numfaces(%i)", numfaces, g_numfaces );
        }
        for ( int i = 0; i < g_numfaces; i++ )
        {
                faceextent_t *e = &g_faceextents[i];
                count = fscanf( f, "%i %i %i %i\n", (int *)&e->mins[0], (int *)&e->mins[1], (int *)&e->maxs[0], (int *)&e->maxs[1] );
                if ( count != 4 )
                {
                        Error( "LoadExtentFile (line %i): scanf failure", i + 2 );
                }
        }
        fclose( f );
        g_faceextents_loaded = true;
}

// =====================================================================================
//  GetFaceExtents (with PLATFORM_CAN_CALC_EXTENT off)
// =====================================================================================
// ZHLT_EMBEDLIGHTMAP: the result of "GetFaceExtents" and the values stored in ".ext" file should always be the original extents;
//                     the new extents of the "?_rad" textures should never appear ("?_rad" textures should be transparent to the tools).
//                     As a consequance, the reported AllocBlock might be inaccurate (usually falsely larger), but it accurately predicts the amount of AllocBlock after the embedded lightmaps are deleted.
void GetFaceExtents( int facenum, int mins_out[2], int maxs_out[2] )
{
        if ( !g_faceextents_loaded )
        {
                Error( "GetFaceExtents: internal error: extent file has not been loaded." );
        }

        faceextent_t *e = &g_faceextents[facenum];
        int i;

        for ( i = 0; i < 2; i++ )
        {
                mins_out[i] = e->mins[i];
                maxs_out[i] = e->maxs[i];
        }
}
#endif

//
// =====================================================================================
//
const int BLOCK_WIDTH = 128;
const int BLOCK_HEIGHT = 128;
typedef struct lightmapblock_s
{
        lightmapblock_s *next;
        bool used;
        int allocated[BLOCK_WIDTH];
}
lightmapblock_t;
void DoAllocBlock( lightmapblock_t *blocks, int w, int h )
{
        lightmapblock_t *block;
        // code from Quake
        int i, j;
        int best, best2;
        int x, y;
        if ( w < 1 || h < 1 )
        {
                Error( "DoAllocBlock: internal error." );
        }
        for ( block = blocks; block; block = block->next )
        {
                best = BLOCK_HEIGHT;
                for ( i = 0; i < BLOCK_WIDTH - w; i++ )
                {
                        best2 = 0;
                        for ( j = 0; j < w; j++ )
                        {
                                if ( block->allocated[i + j] >= best )
                                        break;
                                if ( block->allocated[i + j] > best2 )
                                        best2 = block->allocated[i + j];
                        }
                        if ( j == w )
                        {
                                x = i;
                                y = best = best2;
                        }
                }
                if ( best + h <= BLOCK_HEIGHT )
                {
                        block->used = true;
                        for ( i = 0; i < w; i++ )
                        {
                                block->allocated[x + i] = best + h;
                        }
                        return;
                }
                if ( !block->next )
                { // need to allocate a new block
                        if ( !block->used )
                        {
                                Warning( "CountBlocks: invalid extents %dx%d", w, h );
                                return;
                        }
                        block->next = (lightmapblock_t *)malloc( sizeof( lightmapblock_t ) );
                        hlassume( block->next != NULL, assume_NoMemory );
                        memset( block->next, 0, sizeof( lightmapblock_t ) );
                }
        }
}
int CountBlocks(bspdata_t *data)
{
#if !defined (PLATFORM_CAN_CALC_EXTENT) && !defined (HLRAD)
        return -1; // otherwise GetFaceExtents will error
#endif
        lightmapblock_t *blocks;
        blocks = (lightmapblock_t *)malloc( sizeof( lightmapblock_t ) );
        hlassume( blocks != NULL, assume_NoMemory );
        memset( blocks, 0, sizeof( lightmapblock_t ) );
        int k;
        for ( k = 0; k < data->numfaces; k++ )
        {
                dface_t *f = &data->dfaces[k];
                const texinfo_t *tex = &data->texinfo[f->texinfo];
                const char *texname = GetTextureByNumber( data, f->texinfo );
                contents_t contents = GetTextureContents( texname );
                if ( contents == CONTENTS_SKY //sky, no lightmap allocation.
                     || contents == CONTENTS_WATER //water, no lightmap allocation.
                     || ( data->texinfo[f->texinfo].flags & TEX_SPECIAL ) //aaatrigger, I don't know.
                     )
                {
                        continue;
                }
                int extents[2];
                vec3_t point;
                {
                        int bmins[2];
                        int bmaxs[2];
                        int i;
                        GetFaceExtents( data, k, bmins, bmaxs );
                        for ( i = 0; i < 2; i++ )
                        {
                                extents[i] = ( bmaxs[i] - bmins[i] ) * tex->lightmap_scale;
                        }

                        VectorClear( point );
                        if ( f->numedges > 0 )
                        {
                                int e = data->dsurfedges[f->firstedge];
                                dvertex_t *v = &data->dvertexes[data->dedges[abs( e )].v[e >= 0 ? 0 : 1]];
                                VectorCopy( v->point, point );
                        }
                }
                if ( extents[0] < 0 || extents[1] < 0 || extents[0] > MAX_LIGHTMAP_DIM + 1 || extents[1] > MAX_LIGHTMAP_DIM + 1 )
                        // the default restriction from the engine is 512, but place 'max (512, MAX_LIGHTMAP_DIM * tex->lightmap_scale)' here in case someone raise the limit
                {
                        Warning( "Bad surface extents %d/%d at position (%.0f,%.0f,%.0f)", extents[0], extents[1], point[0], point[1], point[2] );
                        continue;
                }
                DoAllocBlock( blocks, ( extents[0] / tex->lightmap_scale ) + 1, ( extents[1] / tex->lightmap_scale ) + 1 );
        }
        int count = 0;
        lightmapblock_t *next;
        for ( ; blocks; blocks = next )
        {
                if ( blocks->used )
                {
                        count++;
                }
                next = blocks->next;
                free( blocks );
        }
        return count;
}

/*
#ifdef ZHLT_CHART_WADFILES
bool NoWadTextures ()
{
// copied from loadtextures.cpp
int numtextures = g_numtexrefs? ((dtexlump_t *)g_dtexrefs)->numtexref: 0;
for (int i = 0; i < numtextures; i++)
{
int offset = ((dtexlump_t *)g_dtexrefs)->dataofs[i];
int size = g_numtexrefs - offset;
if (offset < 0 || size < (int)sizeof (texref_t))
{
// missing textures have ofs -1
continue;
}
texref_t *mt = (texref_t *)&g_dtexrefs[offset];
if (!mt->offsets[0])
{
return false;
}
}
return true;
}
char *FindWadValue ()
// return NULL for syntax error
// this function needs to be as stable as possible because it might be called from ripent
{
int linestart, lineend;
bool inentity = false;
for (linestart = 0; linestart < g_entdatasize; )
{
for (lineend = linestart; lineend < g_entdatasize; lineend++)
if (g_dentdata[lineend] == '\r' || g_dentdata[lineend] == '\n')
break;
if (lineend == linestart + 1)
{
if (g_dentdata[linestart] == '{')
{
if (inentity)
return NULL;
inentity = true;
}
else if (g_dentdata[linestart] == '}')
{
if (!inentity)
return NULL;
inentity = false;
return _strdup (""); // only parse the first entity
}
else
return NULL;
}
else
{
if (!inentity)
return NULL;
int quotes[4];
int i, j;
for (i = 0, j = linestart; i < 4; i++, j++)
{
for (; j < lineend; j++)
if (g_dentdata[j] == '\"')
break;
if (j >= lineend)
break;
quotes[i] = j;
}
if (i != 4 || quotes[0] != linestart || quotes[3] != lineend - 1)
{
return NULL;
}
if (quotes[1] - (quotes[0] + 1) == (int)strlen ("wad") && !strncmp (&g_dentdata[quotes[0] + 1], "wad", strlen ("wad")))
{
int len = quotes[3] - (quotes[2] + 1);
char *value = (char *)malloc (len + 1);
hlassume (value != NULL, assume_NoMemory);
memcpy (value, &g_dentdata[quotes[2] + 1], len);
value[len] = '\0';
return value;
}
}
for (linestart = lineend; linestart < g_entdatasize; linestart++)
if (g_dentdata[linestart] != '\r' && g_dentdata[linestart] != '\n')
break;
}
return NULL;
}
#endif
*/

#define ENTRIES(a)		(sizeof(a)/sizeof(*(a)))
#define ENTRYSIZE(a)	(sizeof(*(a)))

// =====================================================================================
//  ArrayUsage
//      blah
// =====================================================================================
static int      ArrayUsage( const char* const szItem, const int items, const int maxitems, const int itemsize )
{
        float           percentage = maxitems ? items * 100.0 / maxitems : 0.0;

        Log( "%-13s %7i/%-7i %8i/%-8i (%4.1f%%)\n", szItem, items, maxitems, items * itemsize, maxitems * itemsize, percentage );

        return items * itemsize;
}

// =====================================================================================
//  GlobUsage
//      pritn out global ussage line in chart
// =====================================================================================
static int      GlobUsage( const char* const szItem, const int itemstorage, const int maxstorage )
{
        float           percentage = maxstorage ? itemstorage * 100.0 / maxstorage : 0.0;

        Log( "%-13s    [variable]   %8i/%-8i (%4.1f%%)\n", szItem, itemstorage, maxstorage, percentage );

        return itemstorage;
}

// =====================================================================================
//  PrintBSPFileSizes
//      Dumps info about current file
// =====================================================================================
void            PrintBSPFileSizes(bspdata_t *data)
{
        int             numtextures = data->numtexrefs;
        int             totalmemory = 0;
        int numallocblocks = CountBlocks( data );
        int maxallocblocks = 64;

        Log( "\n" );
        Log( "Object names  Objects/Maxobjs  Memory / Maxmem  Fullness\n" );
        Log( "------------  ---------------  ---------------  --------\n" );

        totalmemory += ArrayUsage( "models", data->nummodels, ENTRIES( data->dmodels ), ENTRYSIZE( data->dmodels ) );
        totalmemory += ArrayUsage( "planes", data->numplanes, MAX_MAP_PLANES, ENTRYSIZE( data->dplanes ) );
        totalmemory += ArrayUsage( "vertexes", data->numvertexes, ENTRIES( data->dvertexes ), ENTRYSIZE( data->dvertexes ) );
        totalmemory += ArrayUsage( "nodes", data->numnodes, ENTRIES( data->dnodes ), ENTRYSIZE( data->dnodes ) );
        totalmemory += ArrayUsage( "texinfos", data->numtexinfo, MAX_MAP_TEXINFO, ENTRYSIZE( data->texinfo ) );
        totalmemory += ArrayUsage( "faces", data->numfaces, ENTRIES( data->dfaces ), ENTRYSIZE( data->dfaces ) );
        //totalmemory += ArrayUsage( "origfaces", data->numorigfaces, ENTRIES( data->dorigfaces ), ENTRYSIZE( data->dorigfaces ) );
        totalmemory += ArrayUsage( "* worldfaces", ( data->nummodels > 0 ? data->dmodels[0].numfaces : 0 ), MAX_MAP_WORLDFACES, 0 );
        totalmemory += ArrayUsage( "leaves", data->numleafs, MAX_MAP_LEAFS, ENTRYSIZE( data->dleafs ) );
        totalmemory += ArrayUsage( "* worldleaves", ( data->nummodels > 0 ? data->dmodels[0].visleafs : 0 ), MAX_MAP_LEAFS_ENGINE, 0 );
        totalmemory += ArrayUsage( "marksurfaces", data->nummarksurfaces, ENTRIES( data->dmarksurfaces ), ENTRYSIZE( data->dmarksurfaces ) );
        totalmemory += ArrayUsage( "surfedges", data->numsurfedges, ENTRIES( data->dsurfedges ), ENTRYSIZE( data->dsurfedges ) );
        totalmemory += ArrayUsage( "edges", data->numedges, ENTRIES( data->dedges ), ENTRYSIZE( data->dedges ) );
        totalmemory += ArrayUsage( "texrefs", data->numtexrefs, ENTRIES( data->dtexrefs ), ENTRYSIZE( data->dtexrefs ) );

        totalmemory += GlobUsage( "lightdata", data->lightdata.size(), g_max_map_lightdata );
        totalmemory += GlobUsage( "visdata", data->visdatasize, sizeof( data->dvisdata ) );
        totalmemory += GlobUsage( "entdata", data->entdatasize, sizeof( data->dentdata ) );
        if ( numallocblocks == -1 )
        {
                Log( "* AllocBlock    [ not available to the " PLATFORM_VERSIONSTRING " version ]\n" );
        }
        else
        {
                totalmemory += ArrayUsage( "* AllocBlock", numallocblocks, maxallocblocks, 0 );
        }

        Log( "%i textures referenced:\n", numtextures );
        for ( int i = 0; i < numtextures; i++ )
        {
                Log( "\t%s\n", data->dtexrefs[i].name );
        }

        Log( "=== Total BSP file data space used: %d bytes ===\n", totalmemory );
}


// =====================================================================================
//  ParseEpair
//      entity key/value pairs
// =====================================================================================
epair_t*        ParseEpair()
{
        epair_t*        e;

        e = (epair_t*)Alloc( sizeof( epair_t ) );

        if ( strlen( g_token ) >= MAX_KEY - 1 )
                Error( "ParseEpair: Key token too long (%i > MAX_KEY)", (int)strlen( g_token ) );

        e->key = _strdup( g_token );
        GetToken( false );

        if ( strlen( g_token ) >= MAX_VAL - 1 ) //MAX_VALUE //vluzacn
                Error( "ParseEpar: Value token too long (%i > MAX_VALUE)", (int)strlen( g_token ) );

        e->value = _strdup( g_token );

        return e;
}

/*
* ================
* ParseEntity
* ================
*/

bool            ParseEntity(bspdata_t *data)
{
        epair_t*        e;
        entity_t*       mapent;

        if ( !GetToken( true ) )
        {
                return false;
        }

        if ( strcmp( g_token, "{" ) )
        {
                Error( "ParseEntity: { not found" );
        }

        if ( data->numentities == MAX_MAP_ENTITIES )
        {
                Error( "data->numentities == MAX_MAP_ENTITIES" );
        }

        mapent = &data->entities[data->numentities];
        data->numentities++;

        while ( 1 )
        {
                if ( !GetToken( true ) )
                {
                        Error( "ParseEntity: EOF without closing brace" );
                }
                if ( !strcmp( g_token, "}" ) )
                {
                        break;
                }
                e = ParseEpair();
                e->next = mapent->epairs;
                mapent->epairs = e;
        }

        // ugly code
        if ( !strncmp( ValueForKey( mapent, "classname" ), "light", 5 ) && *ValueForKey( mapent, "_tex" ) )
        {
                SetKeyValue( mapent, "convertto", ValueForKey( mapent, "classname" ) );
                SetKeyValue( mapent, "classname", "light_surface" );
        }
        if ( !strcmp( ValueForKey( mapent, "convertfrom" ), "light_shadow" )
             || !strcmp( ValueForKey( mapent, "convertfrom" ), "light_bounce" )
             )
        {
                SetKeyValue( mapent, "convertto", ValueForKey( mapent, "classname" ) );
                SetKeyValue( mapent, "classname", ValueForKey( mapent, "convertfrom" ) );
                SetKeyValue( mapent, "convertfrom", "" );
        }
        if ( !strcmp( ValueForKey( mapent, "classname" ), "light_environment" ) &&
             !strcmp( ValueForKey( mapent, "convertfrom" ), "info_sunlight" ) )
        {
                while ( mapent->epairs )
                {
                        DeleteKey( mapent, mapent->epairs->key );
                }
                memset( mapent, 0, sizeof( entity_t ) );
                data->numentities--;
                return true;
        }
        if ( !strcmp( ValueForKey( mapent, "classname" ), "light_environment" ) &&
             IntForKey( mapent, "_fake" ) )
        {
                SetKeyValue( mapent, "classname", "info_sunlight" );
        }

        return true;
}

// =====================================================================================
//  ParseEntities
//      Parses the dentdata string into entities
// =====================================================================================
void            ParseEntities(bspdata_t *data)
{
        data->numentities = 0;
        ParseFromMemory( data->dentdata, data->entdatasize );

        while ( ParseEntity(data) )
        {
        }
}

// =====================================================================================
//  UnparseEntities
//      Generates the dentdata string from all the entities
// =====================================================================================
int anglesforvector( float angles[3], const float vector[3] )
{
        float z = vector[2], r = sqrt( vector[0] * vector[0] + vector[1] * vector[1] );
        float tmp;
        if ( sqrt( z*z + r * r ) < NORMAL_EPSILON )
        {
                return -1;
        }
        else
        {
                tmp = sqrt( z*z + r * r );
                z /= tmp, r /= tmp;
                if ( r < NORMAL_EPSILON )
                {
                        if ( z < 0 )
                        {
                                angles[0] = -90, angles[1] = 0;
                        }
                        else
                        {
                                angles[0] = 90, angles[1] = 0;
                        }
                }
                else
                {
                        angles[0] = atan( z / r ) / Q_PI * 180;
                        float x = vector[0], y = vector[1];
                        tmp = sqrt( x*x + y * y );
                        x /= tmp, y /= tmp;
                        if ( x < -1 + NORMAL_EPSILON )
                        {
                                angles[1] = -180;
                        }
                        else
                        {
                                if ( y >= 0 )
                                {
                                        angles[1] = 2 * atan( y / ( 1 + x ) ) / Q_PI * 180;
                                }
                                else
                                {
                                        angles[1] = 2 * atan( y / ( 1 + x ) ) / Q_PI * 180 + 360;
                                }
                        }
                }
        }
        angles[2] = 0;
        return 0;
}
void            UnparseEntities(bspdata_t *data)
{
        char*           buf;
        char*           end;
        epair_t*        ep;
        char            line[MAXTOKEN];
        int             i;

        buf = data->dentdata;
        end = buf;
        *end = 0;

        for ( i = 0; i < data->numentities; i++ )
        {
                entity_t *mapent = &data->entities[i];
                if ( !strcmp( ValueForKey( mapent, "classname" ), "info_sunlight" ) ||
                     !strcmp( ValueForKey( mapent, "classname" ), "light_environment" ) )
                {
                        float vec[3] = { 0,0,0 };
                        {
                                sscanf( ValueForKey( mapent, "angles" ), "%f %f %f", &vec[0], &vec[1], &vec[2] );
                                float pitch = FloatForKey( mapent, "pitch" );
                                if ( pitch )
                                        vec[0] = pitch;

                                const char *target = ValueForKey( mapent, "target" );
                                if ( target[0] )
                                {
                                        entity_t *targetent = FindTargetEntity( data, target );
                                        if ( targetent )
                                        {
                                                float origin1[3] = { 0,0,0 }, origin2[3] = { 0,0,0 }, normal[3];
                                                sscanf( ValueForKey( mapent, "origin" ), "%f %f %f", &origin1[0], &origin1[1], &origin1[2] );
                                                sscanf( ValueForKey( targetent, "origin" ), "%f %f %f", &origin2[0], &origin2[1], &origin2[2] );
                                                VectorSubtract( origin2, origin1, normal );
                                                anglesforvector( vec, normal );
                                        }
                                }
                        }
                        char stmp[1024];
                        safe_snprintf( stmp, 1024, "%g %g %g", vec[0], vec[1], vec[2] );
                        SetKeyValue( mapent, "angles", stmp );
                        DeleteKey( mapent, "pitch" );

                        if ( !strcmp( ValueForKey( mapent, "classname" ), "info_sunlight" ) )
                        {
                                if ( data->numentities == MAX_MAP_ENTITIES )
                                {
                                        Error( "data->numentities == MAX_MAP_ENTITIES" );
                                }
                                entity_t *newent = &data->entities[data->numentities++];
                                newent->epairs = mapent->epairs;
                                SetKeyValue( newent, "classname", "light_environment" );
                                SetKeyValue( newent, "_fake", "1" );
                                mapent->epairs = NULL;
                        }
                }
        }
        for ( i = 0; i < data->numentities; i++ )
        {
                entity_t *mapent = &data->entities[i];
                if ( !strcmp( ValueForKey( mapent, "classname" ), "light_shadow" )
                     || !strcmp( ValueForKey( mapent, "classname" ), "light_bounce" )
                     )
                {
                        SetKeyValue( mapent, "convertfrom", ValueForKey( mapent, "classname" ) );
                        SetKeyValue( mapent, "classname", ( *ValueForKey( mapent, "convertto" ) ? ValueForKey( mapent, "convertto" ) : "light" ) );
                        SetKeyValue( mapent, "convertto", "" );
                }
        }
        // ugly code
        for ( i = 0; i < data->numentities; i++ )
        {
                entity_t *mapent = &data->entities[i];
                if ( !strcmp( ValueForKey( mapent, "classname" ), "light_surface" ) )
                {
                        if ( !*ValueForKey( mapent, "_tex" ) )
                        {
                                SetKeyValue( mapent, "_tex", "                " );
                        }
                        const char *newclassname = ValueForKey( mapent, "convertto" );
                        if ( !*newclassname )
                        {
                                SetKeyValue( mapent, "classname", "light" );
                        }
                        else if ( strncmp( newclassname, "light", 5 ) )
                        {
                                Error( "New classname for 'light_surface' should begin with 'light' not '%s'.\n", newclassname );
                        }
                        else
                        {
                                SetKeyValue( mapent, "classname", newclassname );
                        }
                        SetKeyValue( mapent, "convertto", "" );
                }
        }
        for ( i = 0; i < data->numentities; i++ )
        {
                ep = data->entities[i].epairs;
                if ( !ep )
                {
                        continue;                                      // ent got removed
                }

                strcat( end, "{\n" );
                end += 2;

                for ( ep = data->entities[i].epairs; ep; ep = ep->next )
                {
                        sprintf( line, "\"%s\" \"%s\"\n", ep->key, ep->value );
                        strcat( end, line );
                        end += strlen( line );
                }
                strcat( end, "}\n" );
                end += 2;

                if ( end > buf + MAX_MAP_ENTSTRING )
                {
                        Error( "Entity text too long" );
                }
        }
        data->entdatasize = end - buf + 1;
}

// =====================================================================================
//  SetKeyValue
//      makes a keyvalue
// =====================================================================================
void			DeleteKey( entity_t* ent, const char* const key )
{
        epair_t **pep;
        for ( pep = &ent->epairs; *pep; pep = &( *pep )->next )
        {
                if ( !strcmp( ( *pep )->key, key ) )
                {
                        epair_t *ep = *pep;
                        *pep = ep->next;
                        Free( ep->key );
                        Free( ep->value );
                        Free( ep );
                        return;
                }
        }
}
void            SetKeyValue( entity_t* ent, const char* const key, const char* const value )
{
        epair_t*        ep;

        if ( !value[0] )
        {
                DeleteKey( ent, key );
                return;
        }
        for ( ep = ent->epairs; ep; ep = ep->next )
        {
                if ( !strcmp( ep->key, key ) )
                {
                        char *value2 = strdup( value );
                        Free( ep->value );
                        ep->value = value2;
                        return;
                }
        }
        ep = (epair_t*)Alloc( sizeof( *ep ) );
        ep->next = ent->epairs;
        ent->epairs = ep;
        ep->key = strdup( key );
        ep->value = strdup( value );
}

// =====================================================================================
//  ValueForKey
//      returns the value for a passed entity and key
// =====================================================================================
const char*     ValueForKey( const entity_t* const ent, const char* const key )
{
        epair_t*        ep;

        for ( ep = ent->epairs; ep; ep = ep->next )
        {
                if ( !strcmp( ep->key, key ) )
                {
                        return ep->value;
                }
        }
        return "";
}

// =====================================================================================
//  IntForKey
// =====================================================================================
int             IntForKey( const entity_t* const ent, const char* const key )
{
        return atoi( ValueForKey( ent, key ) );
}

// =====================================================================================
//  FloatForKey
// =====================================================================================
float           FloatForKey( const entity_t* const ent, const char* const key )
{
        return atof( ValueForKey( ent, key ) );
}

// =====================================================================================
//  GetVectorForKey
//      returns value for key in vec[0-2]
// =====================================================================================
void            GetVectorForKey( const entity_t* const ent, const char* const key, float *vec )
{
        const char*     k;
        float          v1, v2, v3;

        k = ValueForKey( ent, key );
        v1 = v2 = v3 = 0;
        sscanf( k, "%lf %lf %lf", &v1, &v2, &v3 );
        vec[0] = v1;
        vec[1] = v2;
        vec[2] = v3;
}

// =====================================================================================
//  GetVectorDForKey
//      returns value for key in vec[0-2]
// =====================================================================================
void            GetVectorDForKey( const entity_t* const ent, const char* const key, double *vec )
{
	const char*     k;
	double          v1, v2, v3;

	k = ValueForKey( ent, key );
	v1 = v2 = v3 = 0;
	sscanf( k, "%lf %lf %lf", &v1, &v2, &v3 );
	vec[0] = v1;
	vec[1] = v2;
	vec[2] = v3;
}

// =====================================================================================
//  FindTargetEntity
//      
// =====================================================================================
entity_t *FindTargetEntity( bspdata_t *data, const char* const target )
{
        int             i;
        const char*     n;

        for ( i = 0; i < data->numentities; i++ )
        {
                n = ValueForKey( &data->entities[i], "targetname" );
                if ( !strcmp( n, target ) )
                {
                        return &data->entities[i];
                }
        }

        return NULL;
}


void            dtexdata_init()
{
        //g_dlightdata = (colorrgbexp32_t*)AllocBlock( g_max_map_lightdata );
        //hlassume( g_dlightdata != NULL, assume_NoMemory );
}

void CDECL      dtexdata_free()
{
        //FreeBlock( g_dlightdata );
        //g_dlightdata = NULL;
}

// =====================================================================================
//  GetTextureByNumber
//      Touchy function, can fail with a page fault if all the data isnt kosher 
//      (i.e. map was compiled with missing textures)
// =====================================================================================
static char emptystring[1] = { '\0' };
char*           GetTextureByNumber( bspdata_t *data, int texturenumber )
{
        if ( texturenumber == -1 || texturenumber > data->numtexrefs - 1 )
                return emptystring;
        texinfo_t*      info;
        texref_t*		texref;

        info = &data->texinfo[texturenumber];
        texref = &data->dtexrefs[info->texref];

        return texref->name;
}

// =====================================================================================
//  EntityForModel
//      returns entity addy for given modelnum
// =====================================================================================
entity_t*       EntityForModel( bspdata_t *data, const int modnum )
{
        int             i;
        const char*     s;
        char            name[16];

        sprintf( name, "*%i", modnum );
        // search the entities for one using modnum
        for ( i = 0; i < data->numentities; i++ )
        {
                s = ValueForKey( &data->entities[i], "model" );
                if ( !strcmp( s, name ) )
                {
                        return &data->entities[i];
                }
        }

        return &data->entities[0];
}

// =====================================================================================

void SetTextureContents( const char *texname, contents_t contents )
{
        g_tex_contents[texname] = contents;
}

void SetTextureContents( const char *texname, const char *contents )
{
        SetTextureContents( texname, ContentsFromName( contents ) );
}

contents_t ContentsFromName( const char *contents )
{
        if ( !strncmp( contents, "solid", 5 ) )
                return CONTENTS_SOLID;

        else if ( !strncmp( contents, "empty", 5 ) )
                return CONTENTS_EMPTY;

        else if ( !strncmp( contents, "sky", 3 ) )
                return CONTENTS_SKY;

        else if ( !strncmp( contents, "slime", 5 ) )
                return CONTENTS_SLIME;

        else if ( !strncmp( contents, "water", 5 ) )
                return CONTENTS_WATER;

        else if ( !strncmp( contents, "translucent", 11 ) )
                return CONTENTS_TRANSLUCENT;

        else if ( !strncmp( contents, "hint", 4 ) )
                return CONTENTS_HINT;

        else if ( !strncmp( contents, "null", 4 ) )
                return CONTENTS_NULL;

        else if ( !strncmp( contents, "boundingbox", 11 ) )
                return CONTENTS_BOUNDINGBOX;

        else if ( !strncmp( contents, "origin", 6 ) )
                return CONTENTS_ORIGIN;

        return CONTENTS_SOLID;
}

contents_t GetTextureContents( const char *texname )
{
        if ( g_tex_contents.find( texname ) != g_tex_contents.end() )
        {
                return g_tex_contents[texname];
        }

        return CONTENTS_SOLID;
}

// =====================================================================================

#include <simpleHashMap.h>

typedef SimpleHashMap<dface_t *, LRGBColor, pointer_hash> FaceAvgColorCache;
static FaceAvgColorCache face_avg_colors;

/**
 * WARNING: Not thread safe
 * You need to manually wrap a call to this function in a ThreadLock()/ThreadUnlock().
 */
LRGBColor dface_AvgLightColor( bspdata_t *data, dface_t *face, int style )
{
        int idx = face_avg_colors.find( face );
        if ( idx != -1 )
        {
                return face_avg_colors.get_data( idx );
        }

        int luxels = ( face->lightmap_size[0] + 1 ) * ( face->lightmap_size[1] + 1 );
        LRGBColor avg( 0 );
        for ( int i = 0; i < luxels; i++ )
        {
                colorrgbexp32_t col = *SampleLightmap( data, face, i, style, 0 );
                LVector3 vcol( 0 );
                ColorRGBExp32ToVector( col, vcol );
                avg += vcol;
        }
        avg /= luxels;
        face_avg_colors[face] = avg;
        return avg;
}

INLINE colorrgbexp32_t *SampleLightData( pvector<colorrgbexp32_t> &data, const dface_t *face, int ofs, int luxel, int style, int bump )
{
	int luxels = ( face->lightmap_size[0] + 1 ) * ( face->lightmap_size[1] + 1 );
	int bump_count = face->bumped_lightmap ? NUM_BUMP_VECTS + 1 : 1;
	return &data[ofs + ( ( style * bump_count + bump ) * luxels ) + luxel];
}

INLINE colorrgbexp32_t *SampleLightmap( bspdata_t *data, const dface_t *face, int luxel, int style, int bump )
{
	return SampleLightData( data->lightdata, face, face->lightofs, luxel, style, bump );
}

colorrgbexp32_t *SampleSunLightmap( bspdata_t *data, const dface_t *face, int luxel, int style, int bump )
{
	return SampleLightData( data->sunlightdata, face, face->sunlightofs, luxel, style, bump );
}

colorrgbexp32_t *SampleBouncedLightmap( bspdata_t *data, const dface_t *face, int luxel )
{
	return &data->bouncedlightdata[face->bouncedlightofs + luxel];
}

int GetNumWorldLeafs( bspdata_t *bspdata )
{
        return bspdata->dmodels[0].visleafs;
}

LPoint3 VertCoord( const bspdata_t *data, const dface_t *face, int vnum )
{
	int eIndex = data->dsurfedges[face->firstedge + vnum];
	int point;
	if ( eIndex < 0 )
		point = data->dedges[-eIndex].v[1];
	else
		point = data->dedges[eIndex].v[0];
	const dvertex_t *vert = data->dvertexes + point;
	return LPoint3( vert->point[0], vert->point[1], vert->point[2] );
}