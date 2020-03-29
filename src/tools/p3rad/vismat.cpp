#include "vismat.h"
#include "bspfile.h"
#include "cmdlib.h"
#include "log.h"
#include "qrad.h"
#include "trace.h"
#include <bitset>

#define HALFBIT

/*
===================================================================

VISIBILITY MATRIX

Determine which patches can see each other
Use the PVS to accelerate if available
===================================================================
*/

#define TEST_EPSILON	0.1
#define PLANE_TEST_EPSILON  0.01 // patch must be this much in front of the plane to be considered "in front"
#define PATCH_FACE_OFFSET  0.1 // push patch origins off from the face by this amount to avoid self collisions

#define STREAM_SIZE 512

dleaf_t* PointInLeaf( int iNode, LVector3 const& point )
{
        if ( iNode < 0 )
                return &g_bspdata->dleafs[( -1 - iNode )];

        dnode_t *node = &g_bspdata->dnodes[iNode];
        dplane_t *plane = &g_bspdata->dplanes[node->planenum];

        float dist = DotProduct( point, plane->normal ) - plane->dist;
        if ( dist > TEST_EPSILON )
        {
                return PointInLeaf( node->children[0], point );
        }
        else if ( dist <= -TEST_EPSILON )
        {
                return PointInLeaf( node->children[1], point );
        }

        return 0;
}

int ClusterFromPoint( const LVector3 &point )
{
        return PointInLeaf( 0, point ) - g_bspdata->dleafs;
}

void PvsForOrigin( LVector3& org, byte *pvs )
{
        int visofs;
        int cluster;

        if ( !g_bspdata->visdatasize )
        {
                memset( pvs, 255, ( GetNumWorldLeafs(g_bspdata) + 7 ) / 8 );
                return;
        }

        cluster = ClusterFromPoint( org );
        if ( cluster < 0 )
        {
                visofs = -1;
        }
        else
        {
                visofs = g_bspdata->dleafs[cluster].visofs;
        }

        if ( visofs == -1 )
                Error( "visofs == -1" );

        DecompressVis( g_bspdata, &g_bspdata->dvisdata[visofs], pvs, sizeof( pvs ) );
}

void TestPatchToPatch( int patchidx1, int patchidx2, int head, transfer_t *transfers, int thread )
{
        LVector3 tmp;

        //
        // get patches
        //
        if ( patchidx1 == -1 || patchidx2 == -1 )
                return;

        patch_t *patch = &g_patches[patchidx1];
        patch_t *patch2 = &g_patches[patchidx2];

        if ( patch2->child1 != -1 )
        {
                // check to see if we should use a child node instead

                VectorSubtract( patch->origin, patch2->origin, tmp );

                // SQRT(1 / 4)
                // FIXME: should be based on form-factor (ie, include visible angles, etc)
                if ( tmp.dot( tmp ) * 0.0625 < patch2->area )
                {
                        TestPatchToPatch( patchidx1, patch2->child1, head, transfers, thread );
                        TestPatchToPatch( patchidx2, patch2->child2, head, transfers, thread );
                        return;
                }
        }

        // check vis between patch and patch2
        // if bit has not already been set
        //  && v2 is not behind light plane
        //  && v2 is visible from v1
        if ( DotProduct( patch2->origin, patch->normal ) > patch->plane_dist + PLANE_TEST_EPSILON )
        {
                // push out origins from face so that don't intersect their owners
                vec3_t p1, p2;
                VectorAdd( patch->origin, patch->normal, p1 );
                VectorAdd( patch2->origin, patch2->normal, p2 );
                float frac_vis = 0.0;
                int contents = RADTrace::test_line( p1, p2 );
                if ( contents == CONTENTS_EMPTY )
                {
                        // line traced from patch1 to patch2 without hitting anything
                        // create a transfer
                        MakeTransfer( patchidx1, patchidx2, transfers );
                }
        }
}

/*
==============
TestPatchToFace

Sets vis bits for all patches in the face
==============
*/
void TestPatchToFace( unsigned int patchnum, int facenum, int head, transfer_t *transfers, int thread )
{
        if ( g_face_parents[facenum] == -1 || patchnum == -1 )
                return;

        patch_t *patch = &g_patches[patchnum];
        patch_t *patch2 = &g_patches[g_face_parents[facenum]];

        // if emitter is behind that face plane, skip all patches
        patch_t *nextpatch;

        if ( patch2 && DotProduct( patch->origin, patch2->normal ) > patch2->plane_dist + PLANE_TEST_EPSILON )
        {
                // we need to do a real test
                for ( ; patch2; patch2 = nextpatch )
                {
                        //next patch
                        nextpatch = nullptr;
                        if ( patch2->nextparent != -1 )
                        {
                                nextpatch = &g_patches[patch2->nextparent];
                        }

                        int patchidx2 = patch2 - g_patches.data();
                        TestPatchToPatch( patchnum, patchidx2, head, transfers, thread );
                }
        }
}

/*
==============
BuildVisRow

Calc vis bits from a single patch
==============
*/
void BuildVisRow( int patchnum, byte *pvs, int head, transfer_t *transfers, int thread )
{
        int j, k, l, leafidx;
        patch_t *patch;
        dleaf_t *leaf;
        std::bitset<MAX_MAP_FACES> face_tested;
        face_tested.reset();

        patch = &g_patches[patchnum];

        for ( j = 0; j < GetNumWorldLeafs( g_bspdata ); j++ )
        {
                if ( !( pvs[( j ) >> 3] & ( 1 << ( ( j ) & 7 ) ) ) )
                {
                        continue;		// not in pvs
                }

                leaf = g_bspdata->dleafs + j;
                for ( k = 0; k < leaf->nummarksurfaces; k++ )
                {
                        l = g_bspdata->dmarksurfaces[leaf->firstmarksurface + k];
                        // faces can be marksurfed by multiple leaves, but
                        // don't bother testing again
                        if ( face_tested.test( l ) )
                                continue;
                        face_tested.set( l );

                        // don't check patches on the same face
                        if ( patch->facenum == l )
                                continue;

                        TestPatchToFace( patchnum, l, head, transfers, thread );
                }
        }
}

transfer_t *BuildVisLeafs_Start()
{
        return (transfer_t *)calloc( 1, MAX_PATCHES * sizeof( transfer_t ) );
}

void BuildVisLeafs( int threadnum )
{
        transfer_t *transfers = BuildVisLeafs_Start();

        while ( 1 )
        {
                int leaf = GetThreadWork();
                if ( leaf == -1 )
                        break;

                byte pvs[( MAX_MAP_LEAFS + 7 ) / 8];
                patch_t *patch;
                int head;
                unsigned int patchnum;

                DecompressVis( g_bspdata, &g_bspdata->dvisdata[g_bspdata->dleafs[leaf].visofs], pvs, sizeof( pvs ) );
                head = 0;

                // light every patch in the leaf
                if ( g_cluster_children[leaf] != -1 )
                {
                        patch_t *nextpatch;
                        for ( patch = &g_patches[g_cluster_children[leaf]]; patch; patch = nextpatch )
                        {
                                nextpatch = nullptr;
                                if ( patch->nextclusterchild != -1 )
                                {
                                        nextpatch = &g_patches[patch->nextclusterchild];
                                }

                                patchnum = patch - g_patches.data();

                                // build to all other world leafs
                                BuildVisRow( patchnum, pvs, head, transfers, threadnum );

                                // do the transfers
                                MakeScales( patchnum, transfers );
                        }
                }

        }
}

void BuildVisMatrix()
{
        NamedRunThreadsOn( g_bspdata->numleafs, false, BuildVisLeafs );
}

void FreeVisMatrix()
{
}