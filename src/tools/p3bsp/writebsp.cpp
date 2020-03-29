#include "bsp5.h"

//  WriteDrawLeaf
//  WriteFace
//  WriteDrawNodes_r
//  FreeDrawNodes_r
//  WriteDrawNodes
//  BeginBSPFile
//  FinishBSPFile

#include <map>

extern bool g_noopt;
typedef std::map< int, int > texinfomap_t;
static int g_nummappedtexinfo;
static texinfo_t g_mappedtexinfo[MAX_MAP_TEXINFO];
static texinfomap_t g_texinfomap;

// =====================================================================================
//  WritePlane
//  hook for plane optimization
// =====================================================================================
static int WritePlane( int planenum )
{
        planenum = planenum & ( ~1 );
        return planenum;
}


// =====================================================================================
//  WriteTexinfo
// =====================================================================================
static int WriteTexinfo( int texinfo )
{
        if ( texinfo < 0 || texinfo >= g_bspdata->numtexinfo )
        {
                Error( "Bad texinfo number %d.\n", texinfo );
        }

        if ( g_noopt )
        {
                return texinfo;
        }

        texinfomap_t::iterator it;
        it = g_texinfomap.find( texinfo );
        if ( it != g_texinfomap.end() )
        {
                return it->second;
        }

        int c;
        hlassume( g_nummappedtexinfo < MAX_MAP_TEXINFO, assume_MAX_MAP_TEXINFO );
        c = g_nummappedtexinfo;
        g_mappedtexinfo[g_nummappedtexinfo] = g_bspdata->texinfo[texinfo];
        g_texinfomap.insert( texinfomap_t::value_type( texinfo, g_nummappedtexinfo ) );
        g_nummappedtexinfo++;
        return c;
}

// =====================================================================================
//  WriteDrawLeaf
// =====================================================================================
static int		WriteDrawLeaf( node_t *node, const node_t *portalleaf )
{
        face_t**        fp;
        face_t*         f;
        dleaf_t*        leaf_p;
        int				leafnum = g_bspdata->numleafs;

        // emit a leaf
        hlassume( g_bspdata->numleafs < MAX_MAP_LEAFS, assume_MAX_MAP_LEAFS );
        leaf_p = &g_bspdata->dleafs[g_bspdata->numleafs];
        g_bspdata->numleafs++;

        leaf_p->contents = portalleaf->contents;
        leaf_p->flags = 0;

        //
        // write bounding box info
        //
        vec3_t mins, maxs;
#if 0
        printf( "leaf isdetail = %d loosebound = (%f,%f,%f)-(%f,%f,%f) portalleaf = (%f,%f,%f)-(%f,%f,%f)\n", node->isdetail,
                node->loosemins[0], node->loosemins[1], node->loosemins[2], node->loosemaxs[0], node->loosemaxs[1], node->loosemaxs[2],
                portalleaf->mins[0], portalleaf->mins[1], portalleaf->mins[2], portalleaf->maxs[0], portalleaf->maxs[1], portalleaf->maxs[2] );
#endif
        if ( node->isdetail )
        {
                // intersect its loose bounds with the strict bounds of its parent portalleaf
                VectorCompareMaximum( portalleaf->mins, node->loosemins, mins );
                VectorCompareMinimum( portalleaf->maxs, node->loosemaxs, maxs );
        }
        else
        {
                VectorCopy( node->mins, mins );
                VectorCopy( node->maxs, maxs );
        }
        for ( int k = 0; k < 3; k++ )
        {
                leaf_p->mins[k] = (short)qmax( -32767, qmin( (int)mins[k], 32767 ) );
                leaf_p->maxs[k] = (short)qmax( -32767, qmin( (int)maxs[k], 32767 ) );
        }

        leaf_p->visofs = -1;                                   // no vis info yet

                                                               //
                                                               // write the marksurfaces
                                                               //
        //
        // write the leafbrushes
        //
        leaf_p->firstleafbrush = g_bspdata->dleafbrushes.size();
        for ( brush_t *b = node->brushlist; b; b = b->next )
        {
                int brushnum = b->originalbrushnum;
                ThreadLock();
                Verbose( "Leaf %i has brush %i\n", g_bspdata->numleafs - 1, brushnum );
                ThreadUnlock();
                g_bspdata->dleafbrushes.push_back( brushnum );
        }
        leaf_p->numleafbrushes = g_bspdata->dleafbrushes.size() - leaf_p->firstleafbrush;

        
        leaf_p->firstmarksurface = g_bspdata->nummarksurfaces;

        hlassume( node->markfaces != NULL, assume_EmptySolid );

        for ( fp = node->markfaces; *fp; fp++ )
        {
                // emit a marksurface
                f = *fp;
                do
                {
                        // fix face 0 being seen everywhere
                        if ( f->outputnumber == -1 )
                        {
                                f = f->original;
                                continue;
                        }
                        bool ishidden = false;
                        {
                                const char *name = GetTextureByNumber( g_bspdata, f->texturenum );
                                if ( strlen( name ) >= 7 && !strcasecmp( &name[strlen( name ) - 7], "_HIDDEN" ) )
                                {
                                        ishidden = true;
                                }
                        }
                        if ( ishidden )
                        {
                                f = f->original;
                                continue;
                        }
                        g_bspdata->dmarksurfaces[g_bspdata->nummarksurfaces] = f->outputnumber;
                        hlassume( g_bspdata->nummarksurfaces < MAX_MAP_MARKSURFACES, assume_MAX_MAP_MARKSURFACES );
                        g_bspdata->nummarksurfaces++;
                        f = f->original;                               // grab tjunction split faces
                } while ( f );
        }
        free( node->markfaces );

        leaf_p->nummarksurfaces = g_bspdata->nummarksurfaces - leaf_p->firstmarksurface;
        return leafnum;
}

// =====================================================================================
//  WriteFace
// =====================================================================================
static void     WriteFace( face_t* f )
{
        dface_t*        df;
        int             i;
        int             e;

        if ( CheckFaceForHint( f )
             || CheckFaceForSkip( f )
             || CheckFaceForNull( f )  // AJM
             || CheckFaceForDiscardable( f )
             || f->texturenum == -1
             || f->referenced == 0 // this face is not referenced by any nonsolid leaf because it is completely covered by func_details

                                   // =====================================================================================
                                   //Cpt_Andrew - Env_Sky Check
                                   // =====================================================================================
             || CheckFaceForEnv_Sky( f )
             // =====================================================================================

             )
        {
                f->outputnumber = -1;
                return;
        }

        f->outputnumber = g_bspdata->numfaces;

        df = &g_bspdata->dfaces[g_bspdata->numfaces];
        hlassume( g_bspdata->numfaces < MAX_MAP_FACES, assume_MAX_MAP_FACES );
        g_bspdata->numfaces++;

        df->planenum = WritePlane( f->planenum );
        df->side = (byte)(f->planenum & 1);
        df->firstedge = g_bspdata->numsurfedges;
        df->numedges = f->numpoints;
        df->texinfo = WriteTexinfo( f->texturenum );
        df->on_node = (byte)(f->original != NULL);
        df->bumped_lightmap = 0;

        // Save original side/face data


        for ( i = 0; i < f->numpoints; i++ )
        {
                e = f->outputedges[i];
                hlassume( g_bspdata->numsurfedges < MAX_MAP_SURFEDGES, assume_MAX_MAP_SURFEDGES );
                g_bspdata->dsurfedges[g_bspdata->numsurfedges] = e;
                g_bspdata->numsurfedges++;
        }
        free( f->outputedges );
        f->outputedges = NULL;
}

// =====================================================================================
//  WriteDrawNodes_r
// =====================================================================================
static int WriteDrawNodes_r( node_t *node, const node_t *portalleaf )
{
        if ( node->isportalleaf )
        {
                if ( node->contents == CONTENTS_SOLID )
                {
                        return -1;
                }
                else
                {
                        portalleaf = node;
                        // Warning: make sure parent data have not been freed when writing children.
                }
        }
        if ( node->planenum == -1 )
        {
                if ( node->iscontentsdetail )
                {
                        free( node->markfaces );
                        return -1;
                }
                else
                {
                        int leafnum = WriteDrawLeaf( node, portalleaf );
                        return -1 - leafnum;
                }
        }
        dnode_t*        n;
        int             i;
        face_t*         f;
        int nodenum = g_bspdata->numnodes;

        // emit a node
        hlassume( g_bspdata->numnodes < MAX_MAP_NODES, assume_MAX_MAP_NODES );
        n = &g_bspdata->dnodes[g_bspdata->numnodes];
        g_bspdata->numnodes++;

        vec3_t mins, maxs;
#if 0
        if ( node->isdetail || node->isportalleaf )
                printf( "node isdetail = %d loosebound = (%f,%f,%f)-(%f,%f,%f) portalleaf = (%f,%f,%f)-(%f,%f,%f)\n", node->isdetail,
                        node->loosemins[0], node->loosemins[1], node->loosemins[2], node->loosemaxs[0], node->loosemaxs[1], node->loosemaxs[2],
                        portalleaf->mins[0], portalleaf->mins[1], portalleaf->mins[2], portalleaf->maxs[0], portalleaf->maxs[1], portalleaf->maxs[2] );
#endif
        if ( node->isdetail )
        {
                // intersect its loose bounds with the strict bounds of its parent portalleaf
                VectorCompareMaximum( portalleaf->mins, node->loosemins, mins );
                VectorCompareMinimum( portalleaf->maxs, node->loosemaxs, maxs );
        }
        else
        {
                VectorCopy( node->mins, mins );
                VectorCopy( node->maxs, maxs );
        }
        for ( int k = 0; k < 3; k++ )
        {
                n->mins[k] = (short)qmax( -32767, qmin( (int)mins[k], 32767 ) );
                n->maxs[k] = (short)qmax( -32767, qmin( (int)maxs[k], 32767 ) );
        }

        if ( node->planenum & 1 )
        {
                Error( "WriteDrawNodes_r: odd planenum" );
        }
        n->planenum = WritePlane( node->planenum );
        n->firstface = g_bspdata->numfaces;

        for ( f = node->faces; f; f = f->next )
        {
                WriteFace( f );
        }

        n->numfaces = g_bspdata->numfaces - n->firstface;

        //
        // recursively output the other nodes
        //
        for ( i = 0; i < 2; i++ )
        {
                n->children[i] = WriteDrawNodes_r( node->children[i], portalleaf );
        }
        return nodenum;
}

// =====================================================================================
//  FreeDrawNodes_r
// =====================================================================================
static void     FreeDrawNodes_r( node_t* node )
{
        int             i;
        face_t*         f;
        face_t*         next;

        for ( i = 0; i < 2; i++ )
        {
                if ( node->children[i]->planenum != -1 )
                {
                        FreeDrawNodes_r( node->children[i] );
                }
        }

        //
        // free the faces on the node
        //
        for ( f = node->faces; f; f = next )
        {
                next = f->next;
                FreeFace( f );
        }

        free( node );
}

// =====================================================================================
//  WriteDrawNodes
//      Called after a drawing hull is completed
//      Frees all nodes and faces
// =====================================================================================
void OutputEdges_face( face_t *f )
{
        if ( CheckFaceForHint( f )
             || CheckFaceForSkip( f )
             || CheckFaceForNull( f )  // AJM
             || CheckFaceForDiscardable( f )
             || f->texturenum == -1
             || f->referenced == 0
             || CheckFaceForEnv_Sky( f )//Cpt_Andrew - Env_Sky Check
             )
        {
                return;
        }
        f->outputedges = (int *)malloc( f->numpoints * sizeof( int ) );
        hlassume( f->outputedges != NULL, assume_NoMemory );
        int i;
        for ( i = 0; i < f->numpoints; i++ )
        {
                int e = GetEdge( f->pts[i], f->pts[( i + 1 ) % f->numpoints], f );
                f->outputedges[i] = e;
        }
}
int OutputEdges_r( node_t *node, int detaillevel )
{
        int next = -1;
        if ( node->planenum == -1 )
        {
                return next;
        }
        face_t *f;
        for ( f = node->faces; f; f = f->next )
        {
                if ( f->detaillevel > detaillevel )
                {
                        if ( next == -1 ? true : f->detaillevel < next )
                        {
                                next = f->detaillevel;
                        }
                }
                if ( f->detaillevel == detaillevel )
                {
                        OutputEdges_face( f );
                }
        }
        int i;
        for ( i = 0; i < 2; i++ )
        {
                int r = OutputEdges_r( node->children[i], detaillevel );
                if ( r == -1 ? false : next == -1 ? true : r < next )
                {
                        next = r;
                }
        }
        return next;
}
static void RemoveCoveredFaces_r( node_t *node )
{
        if ( node->isportalleaf )
        {
                if ( node->contents == CONTENTS_SOLID )
                {
                        return; // stop here, don't go deeper into children
                }
        }
        if ( node->planenum == -1 )
        {
                // this is a leaf
                if ( node->iscontentsdetail )
                {
                        return;
                }
                else
                {
                        face_t **fp;
                        for ( fp = node->markfaces; *fp; fp++ )
                        {
                                for ( face_t *f = *fp; f; f = f->original ) // for each tjunc subface
                                {
                                        f->referenced++; // mark the face as referenced
                                }
                        }
                }
                return;
        }

        // this is a node
        for ( face_t *f = node->faces; f; f = f->next )
        {
                f->referenced = 0; // clear the mark
        }

        RemoveCoveredFaces_r( node->children[0] );
        RemoveCoveredFaces_r( node->children[1] );
}
void            WriteDrawNodes( node_t* headnode )
{
        RemoveCoveredFaces_r( headnode ); // fill "referenced" value
                                          // higher detail level should not compete for edge pairing with lower detail level.
        int detaillevel, nextdetaillevel;
        for ( detaillevel = 0; detaillevel != -1; detaillevel = nextdetaillevel )
        {
                nextdetaillevel = OutputEdges_r( headnode, detaillevel );
        }
        WriteDrawNodes_r( headnode, NULL );
}


// =====================================================================================
//  BeginBSPFile
// =====================================================================================
void            BeginBSPFile()
{
        // these values may actually be initialized
        // if the file existed when loaded, so clear them explicitly
        g_nummappedtexinfo = 0;
        g_texinfomap.clear();
        g_bspdata->nummodels = 0;
        g_bspdata->numfaces = 0;
        g_bspdata->numnodes = 0;
        g_bspdata->numvertexes = 0;
        g_bspdata->nummarksurfaces = 0;
        g_bspdata->numsurfedges = 0;

        // edge 0 is not used, because 0 can't be negated
        g_bspdata->numedges = 1;

        // leaf 0 is common solid with no faces
        g_bspdata->numleafs = 1;
        g_bspdata->dleafs[0].contents = CONTENTS_SOLID;
}

// =====================================================================================
//  FinishBSPFile
// =====================================================================================
void            FinishBSPFile()
{
        Verbose( "--- FinishBSPFile ---\n" );

        if ( g_bspdata->dmodels[0].visleafs > MAX_MAP_LEAFS_ENGINE )
        {
                Warning( "Number of world leaves(%d) exceeded MAX_MAP_LEAFS(%d)\nIf you encounter problems when running your map, consider this the most likely cause.\n", g_bspdata->dmodels[0].visleafs, MAX_MAP_LEAFS_ENGINE );
        }
        if ( g_bspdata->dmodels[0].numfaces > MAX_MAP_WORLDFACES )
        {
                Warning( "Number of world faces(%d) exceeded %d. Some faces will disappear in game.\nTo reduce world faces, change some world brushes (including func_details) to func_walls.\n", g_bspdata->dmodels[0].numfaces, MAX_MAP_WORLDFACES );
        }
        if ( !g_noopt )
        {
                {
                        Log( "Reduced %d texinfos to %d\n", g_bspdata->numtexinfo, g_nummappedtexinfo );
                        for ( int i = 0; i < g_nummappedtexinfo; i++ )
                        {
                                g_bspdata->texinfo[i] = g_mappedtexinfo[i];
                        }
                        g_bspdata->numtexinfo = g_nummappedtexinfo;
                }
        }
        else
        {
                hlassume( g_bspdata->numtexinfo < MAX_MAP_TEXINFO, assume_MAX_MAP_TEXINFO );
        }

        hlassume( g_bspdata->numplanes < MAX_MAP_PLANES, assume_MAX_MAP_PLANES );

#ifdef PLATFORM_CAN_CALC_EXTENT
        WriteExtentFile( g_bspdata, g_extentfilename );
#else
        Warning( "The " PLATFORM_VERSIONSTRING " version of hlbsp couldn't create extent file. The lack of extent file may cause hlrad error." );
#endif
        if ( g_chart )
        {
                PrintBSPFileSizes( g_bspdata );
        }

        Log( "Writing %d planes\n", g_bspdata->numplanes );

        WriteBSPFile( g_bspdata, g_bspfilename );
}
