#include "lightmap.h"
#include "lights.h"
#include "radial.h"
#include "halton.h"
#include "anorms.h"
#include "bsptools.h"
#include "trace.h"

#include <CL/cl.h>

#include <clockObject.h>

edgeshare_t     edgeshare[MAX_MAP_EDGES];
faceneighbor_t  faceneighbor[MAX_MAP_FACES];
int             vertexref[MAX_MAP_VERTS];
int*            vertexface[MAX_MAP_VERTS];

vec3_t          g_face_centroids[MAX_MAP_EDGES]; // BUG: should this be [MAX_MAP_FACES]?
bool            g_sky_lighting_fix = DEFAULT_SKY_LIGHTING_FIX;

#define VectorOut(vec) vec[0] << ", " << vec[1] << ", " << vec[2]

int EdgeVertex( dface_t *f, int edge )
{
        int k;
        if ( edge < 0 )
                edge += f->numedges;
        else if ( edge >= f->numedges )
                edge = edge % f->numedges;

        k = g_bspdata->dsurfedges[f->firstedge + edge];
        if ( k < 0 )
        {
                return g_bspdata->dedges[-k].v[1];
        }
        else
        {
                return g_bspdata->dedges[k].v[0];
        }
}

unsigned short FindOrAddNormal( const LVector3 &normal )
{
        for ( auto itr = g_bspdata->vertnormals.begin(); itr != g_bspdata->vertnormals.end(); itr++ )
        {
                if ( VectorCompare( normal.get_data(), itr->point) )
                {
                        return itr - g_bspdata->vertnormals.begin();
                }
        }

        // normal not found, add it
        unsigned short idx = g_bspdata->vertnormals.size();
        dvertex_t vert;
        VectorCopy( normal, vert.point );
        g_bspdata->vertnormals.push_back( vert );
        return idx;
}

void SaveVertexNormals()
{
        g_bspdata->vertnormalindices.clear();
        g_bspdata->vertnormals.clear();

        for ( int i = 0; i < g_bspdata->numfaces; i++ )
        {
                faceneighbor_t *fn = &faceneighbor[i];
                dface_t *f = &g_bspdata->dfaces[i];

                for ( int j = 0; j < f->numedges; j++ )
                {

                        LVector3 normal( 0 );

                        if ( fn->normal )
                                normal = fn->normal[j];

                        unsigned short idx = FindOrAddNormal( normal );
                        g_bspdata->vertnormalindices.push_back( idx );
                }
        }
}

void PairEdges()
{
        int     i, j, k, n, m;
        dface_t *f;
        int     numneighbors;
        int     tmpneighbor[64];
        faceneighbor_t  *fn;

        // count number of faces that reference each vertex
        for ( i = 0, f = g_bspdata->dfaces; i < g_bspdata->numfaces; i++, f++ )
        {
                for ( j = 0; j < f->numedges; j++ )
                {
                        vertexref[EdgeVertex( f, j )]++;
                }
        }

        // allocate room
        for ( i = 0; i < g_bspdata->numvertexes; i++ )
        {
                // use the count from above to allocate a big enough array
                vertexface[i] = (int *)calloc( vertexref[i], sizeof( vertexface[0] ) );
                // clear the temp data
                vertexref[i] = 0;
        }

        // store a list of every face that uses a particular vertex
        for ( i = 0, f = g_bspdata->dfaces; i < g_bspdata->numfaces; i++, f++ )
        {
                for ( j = 0; j < f->numedges; j++ )
                {
                        n = EdgeVertex( f, j );

                        for ( k = 0; k < vertexref[n]; k++ )
                        {
                                if ( vertexface[n][k] == i )
                                        break;
                        }

                        if ( k >= vertexref[n] )
                        {
                                // add the face to the list
                                vertexface[n][k] = i;
                                vertexref[n]++;
                        }
                }
        }

        // calc normals
        for ( i = 0, f = g_bspdata->dfaces; i < g_bspdata->numfaces; f++, i++ )
        {
                fn = &faceneighbor[i];

                const dplane_t *pl = getPlaneFromFace( f );

                // get face normal
                VectorCopy( pl->normal, fn->facenormal );
        }

        // find neighbors
        for ( i = 0, f = g_bspdata->dfaces; i < g_bspdata->numfaces; i++, f++ )
        {
                numneighbors = 0;
                fn = &faceneighbor[i];

                // allocate room for vertex normals
                fn->normal = (LVector3 *)calloc( f->numedges, sizeof( fn->normal[0] ) );

                // look up all faces sharing vertices and add them to the list
                for ( j = 0; j < f->numedges; j++ )
                {
                        n = EdgeVertex( f, j );
                        for ( k = 0; k < vertexref[n]; k++ )
                        {
                                double cos_normal_angle;
                                LVector3 *neighbornormal;

                                // skip self
                                if ( vertexface[n][k] == i )
                                        continue;

                                neighbornormal = &faceneighbor[vertexface[n][k]].facenormal;
                                cos_normal_angle = fn->facenormal.dot( *neighbornormal );

                                // TODO: smoothing groups
                                if ( cos_normal_angle >= g_smoothing_threshold )
                                {
                                        fn->normal[j] += *neighbornormal;
                                }
                                else
                                {
                                        // not considered a neighbor
                                        continue;
                                }

                                // look to see if we've already added this one
                                for ( m = 0; m < numneighbors; m++ )
                                {
                                        if ( tmpneighbor[m] == vertexface[n][k] )
                                                break;
                                }

                                if ( m >= numneighbors )
                                {
                                        // add to neighbor list
                                        tmpneighbor[m] = vertexface[n][k];
                                        numneighbors++;
                                        if ( numneighbors > ARRAYSIZE( tmpneighbor ) )
                                        {
                                                Error( "Stack overflow in neighbors\n" );
                                        }
                                }
                        }
                }

                if ( numneighbors )
                {
                        // copy over neighbor list
                        fn->numneighbors = numneighbors;
                        fn->neighbor = (int *)calloc( numneighbors, sizeof( fn->neighbor[0] ) );
                        for ( m = 0; m < numneighbors; m++ )
                        {
                                fn->neighbor[m] = tmpneighbor[m];
                        }
                }

                // fixup normals
                for ( j = 0; j < f->numedges; j++ )
                {
                        fn->normal[j] += fn->facenormal;
                        fn->normal[j].normalize();
                }
        }
}

// =====================================================================================
//  TextureNameFromFace
// =====================================================================================
static const char* TextureNameFromFace( const dface_t* const f )
{
        texinfo_t*      tx;
        texref_t*       tref;

        //
        // check for light emited by texture
        //
        tx = &g_bspdata->texinfo[f->texinfo];
        tref = &g_bspdata->dtexrefs[tx->texref];

        return tref->name;
}

// =====================================================================================
//  CalcFaceExtents
//      Fills in s->texmins[] and s->texsize[]
//      also sets exactmins[] and exactmaxs[]
// =====================================================================================
static void     CalcFaceExtents( lightinfo_t* l )
{
        const int       facenum = l->surfnum;
        dface_t*        s;
        float           mins[2], maxs[2], val; //vec_t           mins[2], maxs[2], val; //vluzacn
        int             i, j, e;
        dvertex_t*      v;
        texinfo_t*      tex;

        s = l->face;

        mins[0] = mins[1] = 99999999;
        maxs[0] = maxs[1] = -99999999;

        tex = &g_bspdata->texinfo[s->texinfo];

        for ( i = 0; i < s->numedges; i++ )
        {
                e = g_bspdata->dsurfedges[s->firstedge + i];
                if ( e >= 0 )
                {
                        v = g_bspdata->dvertexes + g_bspdata->dedges[e].v[0];
                }
                else
                {
                        v = g_bspdata->dvertexes + g_bspdata->dedges[-e].v[1];
                }

                for ( j = 0; j < 2; j++ )
                {
                        val = v->point[0] * tex->lightmap_vecs[j][0] +
                                v->point[1] * tex->lightmap_vecs[j][1] + v->point[2] * tex->lightmap_vecs[j][2] + tex->lightmap_vecs[j][3];
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
                l->exactmins[i] = mins[i];
                l->exactmaxs[i] = maxs[i];

        }
        int bmins[2];
        int bmaxs[2];
        GetFaceExtents( g_bspdata, l->surfnum, bmins, bmaxs );
        for ( i = 0; i < 2; i++ )
        {
                mins[i] = bmins[i];
                maxs[i] = bmaxs[i];
                l->texmins[i] = bmins[i];
                l->texsize[i] = bmaxs[i] - bmins[i];
                // Also emit this data to the BSP faces themselves.
                s->lightmap_mins[i] = l->texmins[i];
                s->lightmap_size[i] = l->texsize[i];
        }

        if ( !( tex->flags & TEX_SPECIAL ) )
        {
                if ( ( l->texsize[0] > MAX_LIGHTMAP_DIM + 1 ) || ( l->texsize[1] > MAX_LIGHTMAP_DIM + 1 )
                     || l->texsize[0] < 0 || l->texsize[1] < 0 //--vluzacn
                     )
                {
                        ThreadLock();
                        PrintOnce( "\nfor Face %d (texture %s) at ", s - g_bspdata->dfaces, TextureNameFromFace( s ) );

                        for ( i = 0; i < s->numedges; i++ )
                        {
                                e = g_bspdata->dsurfedges[s->firstedge + i];
                                if ( e >= 0 )
                                {
                                        v = g_bspdata->dvertexes + g_bspdata->dedges[e].v[0];
                                }
                                else
                                {
                                        v = g_bspdata->dvertexes + g_bspdata->dedges[-e].v[1];
                                }
                                vec3_t pos;
                                VectorAdd( v->point, g_face_offset[facenum], pos );
                                Log( "(%4.3f %4.3f %4.3f) ", pos[0], pos[1], pos[2] );
                        }
                        Log( "\n" );

                        Error( "Bad surface extents (%d x %d)\nCheck the file ZHLTProblems.html for a detailed explanation of this problem", l->texsize[0], l->texsize[1] );
                }
        }
}

// =====================================================================================
//  CalcFaceVectors
//      Fills in texorg, worldtotex. and textoworld
// =====================================================================================
static void     CalcFaceVectors( lightinfo_t* l )
{
        texinfo_t*      tex;
        int             i, j;

        tex = &g_bspdata->texinfo[l->face->texinfo];

        // convert from float to double
        for ( i = 0; i < 2; i++ )
        {
                for ( j = 0; j < 3; j++ )
                {
                        l->worldtotex[i][j] = tex->lightmap_vecs[i][j];
                }
        }

        vec3_t luxel_cross;
        CrossProduct( tex->lightmap_vecs[1], tex->lightmap_vecs[0], luxel_cross );

        float det = -DotProduct( l->facenormal, luxel_cross );
        if ( fabs( det ) < 1.0e-20 )
        {
                Warning( "warning - face vectors parallel to face normal. bad lighting will be produced\n" );
                memset( l->texorg, 0, sizeof( vec3_t ) );
        }
        else
        {
                // invert the matrix
                l->textoworld[0][0] = ( l->facenormal[2] * l->worldtotex[1][1] - l->facenormal[1] * l->worldtotex[1][2] ) / det;
                l->textoworld[1][0] = ( l->facenormal[1] * l->worldtotex[0][2] - l->facenormal[2] * l->worldtotex[0][1] ) / det;
                l->texorg[0] = -( l->facedist * luxel_cross[0] ) / det;
                l->textoworld[0][1] = ( l->facenormal[0] * l->worldtotex[1][2] - l->facenormal[2] * l->worldtotex[1][0] ) / det;
                l->textoworld[1][1] = ( l->facenormal[2] * l->worldtotex[0][0] - l->facenormal[0] * l->worldtotex[0][2] ) / det;
                l->texorg[1] = -( l->facedist * luxel_cross[1] ) / det;
                l->textoworld[0][2] = ( l->facenormal[1] * l->worldtotex[1][0] - l->facenormal[0] * l->worldtotex[1][1] ) / det;
                l->textoworld[1][2] = ( l->facenormal[0] * l->worldtotex[0][1] - l->facenormal[1] * l->worldtotex[0][0] ) / det;
                l->texorg[2] = -( l->facedist * luxel_cross[2] ) / det;

                // Adjust for luxel offset
                VectorMA( l->texorg, -tex->lightmap_vecs[0][3], l->textoworld[0], l->texorg );
                VectorMA( l->texorg, -tex->lightmap_vecs[1][3], l->textoworld[1], l->texorg );
        }
        // compensate for org'd bmodels
        //VectorAdd(l->texorg, l->)
        // whoops
}

// =====================================================================================
//  CalcPoints
//      For each texture aligned grid point, back project onto the plane
//      to get the world xyz value of the sample point
// =====================================================================================

//==============================================================

facelight_t facelight[MAX_MAP_FACES];
lightinfo_t *lightinfo;

// =====================================================================================
//  GetPhongNormal
// =====================================================================================
void            GetPhongNormal( int facenum, const LVector3 &spot, LVector3 &phongnormal )
{
        int j;
        dface_t *f = g_bspdata->dfaces + facenum;
        LVector3 facenormal, vspot;

        const dplane_t *pl = getPlaneFromFace( f );

        VectorCopy( pl->normal, facenormal );
        VectorCopy( facenormal, phongnormal );

        if ( g_smoothing_threshold != 1 )
        {
                faceneighbor_t *fn = &faceneighbor[facenum];

                // Calculate modified point normal for surface
                // Use the edge normals iff they are defined.  Bend the surface towards the edge normal(s)
                // Crude first attempt: find nearest edge normal and do a simple interpolation with facenormal.
                // Second attempt: find edge points+center that bound the point and do a three-point triangulation(baricentric)
                // Better third attempt: generate the point normals for all vertices and do baricentric triangulation.

                for ( j = 0; j < f->numedges; j++ )
                {
                        LVector3	v1, v2;
                        float		a1, a2, aa, bb, ab;
                        int			vert1, vert2;

                        LVector3& n1 = fn->normal[j];
                        LVector3& n2 = fn->normal[( j + 1 ) % f->numedges];

                        vert1 = EdgeVertex( f, j );
                        vert2 = EdgeVertex( f, j + 1 );

                        LVector3 p1 = GetLVector3_2( g_bspdata->dvertexes[vert1].point );
                        LVector3 p2 = GetLVector3_2( g_bspdata->dvertexes[vert2].point );

                        // Build vectors from the middle of the face to the edge vertexes and the sample pos.
                        VectorSubtract( p1, g_face_centroids[facenum], v1 );
                        VectorSubtract( p2, g_face_centroids[facenum], v2 );
                        VectorSubtract( spot, g_face_centroids[facenum], vspot );
                        aa = DotProduct( v1, v1 );
                        bb = DotProduct( v2, v2 );
                        ab = DotProduct( v1, v2 );
                        a1 = ( bb * DotProduct( v1, vspot ) - ab * DotProduct( vspot, v2 ) ) / ( aa * bb - ab * ab );
                        a2 = ( DotProduct( vspot, v2 ) - a1 * ab ) / bb;

                        // Test center to sample vector for inclusion between center to vertex vectors (Use dot product of vectors)
                        if ( a1 >= 0.0 && a2 >= 0.0 )
                        {
                                // calculate distance from edge to pos
                                LVector3	temp;
                                float scale;

                                // Interpolate between the center and edge normals based on sample position
                                scale = 1.0 - a1 - a2;
                                VectorScale( fn->facenormal, scale, phongnormal );
                                VectorScale( n1, a1, temp );
                                VectorAdd( phongnormal, temp, phongnormal );
                                VectorScale( n2, a2, temp );
                                VectorAdd( phongnormal, temp, phongnormal );
                                nassertv( VectorLength( phongnormal ) > 1.0e-20 );
                                phongnormal.normalize();
                                return;
                        }
                }
        }
}

void GetBumpNormals( const texinfo_t *texinfo, const LVector3 &face_normal,
                     const LVector3 &phong_normal, LVector3 *bump_vecs )
{
        LVector3 stmp( texinfo->vecs[0][0], texinfo->vecs[0][1], texinfo->vecs[0][2] );
        LVector3 ttmp( texinfo->vecs[1][0], texinfo->vecs[1][1], texinfo->vecs[1][2] );
        GetBumpNormals( stmp, ttmp, face_normal, phong_normal, bump_vecs );
}

// =====================================================================================
//  BuildFacelights
// =====================================================================================

void InitLightInfo( lightinfo_t &l, int facenum )
{
        l.isflat = true;

        dface_t *f = g_bspdata->dfaces + facenum;
        const dplane_t *plane = getPlaneFromFace( f );

        facelight_t *fl = &facelight[facenum];
        
        if ( g_face_patches[facenum] != -1 )
                l.bumped = g_patches[g_face_patches[facenum]].bumped;
        else
                l.bumped = false;

        fl->normal_count = l.bumped ? NUM_BUMP_VECTS + 1 : 1;
        l.normal_count = fl->normal_count;
        fl->bumped = l.bumped;
        f->bumped_lightmap = l.bumped;
        l.surfnum = facenum;
        l.face = f;

        //VectorCopy( g_translucenttextures[g_bspdata->texinfo[f->texinfo].texref], l.translucent_v );
        //l.translucent_b = !VectorCompare( l.translucent_v, vec3_origin );
        l.texref = g_bspdata->texinfo[f->texinfo].texref;

        VectorCopy( plane->normal, l.facenormal );

        //
        // rotate plane
        //

        l.facedist = plane->dist;

        CalcFaceVectors( &l );
        CalcFaceExtents( &l );

        if ( g_smoothing_threshold != 1 )
        {
                faceneighbor_t *fn = &faceneighbor[facenum];

                for ( int j = 0; j < f->numedges; j++ )
                {
                        float dot = DotProduct( l.facenormal, fn->normal[j] );
                        if ( dot < 1.0 - EQUAL_EPSILON )
                        {
                                l.isflat = false;
                                break;
                        }
                }
        }
}

void InitSampleInfo( const lightinfo_t &l, int thread, SSE_SampleInfo_t &info )
{
        info.lightmap_width = l.face->lightmap_size[0] + 1;
        info.lightmap_height = l.face->lightmap_size[1] + 1;
        info.lightmap_size = info.lightmap_width * info.lightmap_height;

        info.texinfo = &g_bspdata->texinfo[l.face->texinfo];
        info.normal_count = l.bumped ? NUM_BUMP_VECTS + 1 : 1;
        info.facenum = l.surfnum;
        info.face = l.face;
        info.facelight = &facelight[info.facenum];
        info.thread = thread;
        info.num_samples = info.facelight->numsamples;
        info.num_sample_groups = ( info.num_samples & 0x3 ) ? ( info.num_samples / 4 ) + 1 : ( info.num_samples / 4 );

        // initialize normals if the surface is flat
        if ( l.isflat )
        {
                info.point_normals[0].DuplicateVector( l.facenormal );

                // use facenormal along with the smooth normal to build the three bump map vectors
                if ( info.normal_count > 1 )
                {
                        LVector3 bump_vects[NUM_BUMP_VECTS];
                        LVector3 fn3 = GetLVector3( l.facenormal );
                        GetBumpNormals( info.texinfo, fn3, fn3, bump_vects );

                        for ( int b = 0; b < NUM_BUMP_VECTS; b++ )
                        {
                                info.point_normals[b + 1].DuplicateVector( bump_vects[b] );
                        }
                }
        }
}

bool BuildSamplesAndLuxels_DoFast( lightinfo_t *l, facelight_t *fl, int facenum )
{
        return false;
}

Winding *WindingFromFace( dface_t *face )
{
        return new Winding( *face, g_bspdata );
}

Winding *LightmapCoordWindingForFace( lightinfo_t *l )
{
        Winding *w = WindingFromFace( l->face );
        for ( int i = 0; i < w->m_NumPoints; i++ )
        {
                LVector2 coord;
                WorldToLuxelSpace( l, GetLVector3( w->m_Points[i] ), coord );
                w->m_Points[i][0] = coord[0];
                w->m_Points[i][1] = coord[1];
                w->m_Points[i][2] = 0.0;
        }

        return w;
}

bool BuildSamples( lightinfo_t *l, facelight_t *fl, int facenum )
{
        // lightmap size
        int width = l->face->lightmap_size[0] + 1;
        int height = l->face->lightmap_size[1] + 1;

        // ratio of world area / lightmap area
        texinfo_t *tex = g_bspdata->texinfo + l->face->texinfo;
        fl->world_area_per_luxel = 1.0 / ( std::sqrt( DotProduct( tex->lightmap_vecs[0],
                                                                  tex->lightmap_vecs[0] ) ) *
                                           std::sqrt( DotProduct( tex->lightmap_vecs[1],
                                                                  tex->lightmap_vecs[1] ) ) );

        // allocate a large number of samples for creation -- get copied later!
        pvector<sample_t> sample_data;
        sample_data.resize( MAX_SINGLEMAP * 2 );
        sample_t *samples = sample_data.data();

        // lightmap space winding
        Winding *lightmap_winding = LightmapCoordWindingForFace( l );

        //
        // build vector pointing along the lightmap cutting planes
        //
        LVector3 snorm( 1.0, 0.0, 0.0 );
        LVector3 tnorm( 0.0, 1.0, 0.0 );

        float sample_offset = 1.0;

        //
        // clip the lightmap "spaced" winding by the lightmap cutting planes
        //
        Winding *winding_t1, *winding_t2;
        Winding *winding_s1, *winding_s2;
        float dist;

        for ( int t = 0; t < height && lightmap_winding; t++ )
        {
                dist = t + sample_offset;

                // lop off a sample in the t dimension
                // hack - need a separate epsilon for lightmap space since ON_EPSILON is for texture space
                vec3_t vtnorm;
                VectorCopy( tnorm, vtnorm );
                lightmap_winding->Clip( vtnorm, dist, &winding_t1, &winding_t2, ON_EPSILON / 16.0f );

                for ( int s = 0; s < width && winding_t2; s++ )
                {
                        dist = s + sample_offset;

                        vec3_t vsnorm;
                        VectorCopy( snorm, vsnorm );
                        // lop off a sample in the s dimension, and put it in ws2
                        // hack - need a separate epsilon for lightmap space since ON_EPSILON is for texture space
                        winding_t2->Clip( vsnorm, dist, &winding_s1, &winding_s2, ON_EPSILON / 16.0 );

                        //
                        // s2 winding is a single sample worth of winding
                        //
                        if ( winding_s2 )
                        {
                                // save the s, t positions
                                samples->s = s;
                                samples->t = t;

                                // get the lightmap space area of ws2 and convert to world area
                                // and find the center (then convert it to 2D)
                                vec3_t center;
                                samples->area = winding_s2->getAreaAndBalancePoint( center ) * fl->world_area_per_luxel;
                                samples->coord[0] = center[0];
                                samples->coord[1] = center[1];

                                // find winding bounds (then convert it to 2D)
                                vec3_t mins, maxs;
                                winding_s2->getBounds( mins, maxs );
                                samples->mins[0] = mins[0];
                                samples->mins[1] = mins[1];
                                samples->maxs[0] = maxs[0];
                                samples->maxs[1] = maxs[1];

                                // convert from lightmap space to world space
                                LVector3 vpos;
                                LuxelToWorldSpace( l, samples->coord[0], samples->coord[1], vpos );
                                VectorCopy( vpos, samples->pos );

                                if ( g_extra && samples->area < fl->world_area_per_luxel - EQUAL_EPSILON )
                                {
                                        //
                                        // convert the winding from lightmaps space to world for debug rendering
                                        // and sub-sampling
                                        //

                                        LVector3 worldpos;
                                        for ( int pt = 0; pt < winding_s2->m_NumPoints; pt++ )
                                        {
                                                LuxelToWorldSpace( l, winding_s2->m_Points[pt][0], winding_s2->m_Points[pt][1], worldpos );
                                                VectorCopy( worldpos, winding_s2->m_Points[pt] );
                                        }
                                        samples->winding = winding_s2;
                                }
                                else
                                {
                                        // winding isn't needed, free it
                                        samples->winding = nullptr;
                                        delete winding_s2;
                                        winding_s2 = nullptr;
                                }

                                samples++;
                        }

                        //
                        // if winding T2 still exists free it and set it equal to S1 (the rest of the row minus the sample just created)
                        //
                        if ( winding_t2 )
                        {
                                delete winding_t2;
                                winding_t2 = nullptr;
                        }

                        // clip the rest of "s"
                        winding_t2 = winding_s1;
                }

                //
                // if the original lightmap winding exists free it and set it equal to T1 (the rest of the winding not cut into samples)
                //
                if ( lightmap_winding )
                {
                        delete lightmap_winding;
                        lightmap_winding = nullptr;
                }
                if ( winding_t2 )
                {
                        delete winding_t2;
                        winding_t2 = nullptr;
                }

                lightmap_winding = winding_t1;
        }

        //
        // copy over samples
        //
        fl->numsamples = samples - sample_data.data();
        fl->sample = (sample_t *)calloc( fl->numsamples, sizeof( *fl->sample ) );
        if ( !fl->sample )
        {
                return false;
        }

        memcpy( fl->sample, sample_data.data(), fl->numsamples * sizeof( *fl->sample ) );

        // supply a default sample normal (face normal - assumed flat)
        for ( int sample = 0; sample < fl->numsamples; sample++ )
        {
                nassertr( VectorLength( l->facenormal ) > 1e-20, false );
                VectorCopy( l->facenormal, fl->sample[sample].normal );
        }

        // statistics - warning?!
        if ( fl->numsamples == 0 )
        {
                Log( "no samples %d\n", l->face - g_bspdata->dfaces );
        }

        return true;
}

bool BuildLuxels( lightinfo_t *l, facelight_t *fl, int facenum )
{
        // lightmap size
        int width = l->face->lightmap_size[0] + 1;
        int height = l->face->lightmap_size[1] + 1;

        // calculate actual luxel points
        fl->numluxels = width * height;
        fl->luxel_points = (vec3_t *)calloc( fl->numluxels, sizeof( *fl->luxel_points ) );
        if ( !fl->luxel_points )
        {
                return false;
        }

        for ( int t = 0; t < height; t++ )
        {
                for ( int s = 0; s < width; s++ )
                {
                        LVector3 world;
                        LuxelToWorldSpace( l, s, t, world );
                        VectorCopy( world, fl->luxel_points[s + t * width] );
                }
        }

        return true;
}

void CalcPoints( lightinfo_t *l, facelight_t *fl, int facenum )
{
        // quick and dirty
        if ( g_fastmode )
        {
                if ( !BuildSamplesAndLuxels_DoFast( l, fl, facenum ) )
                {
                        Log( "Face %d: (Fast) Error building samples and luxels\n", facenum );
                }
                return;
        }

        // build the samples
        if ( !BuildSamples( l, fl, facenum ) )
        {
                Log( "Face %d: Error building samples!\n", facenum );
        }

        // build the luxels
        if ( !BuildLuxels( l, fl, facenum ) )
        {
                Log( "Face %d: Error building luxels!\n", facenum );
        }
}

void AllocateLightstyleSamples( facelight_t *fl, int style, int normal_count )
{
        fl->light[style] = (bumpsample_t *)calloc( fl->numsamples, sizeof( bumpsample_t ) );
	fl->sunlight[style] = (bumpsample_t *)calloc( fl->numsamples, sizeof( bumpsample_t ) );
}

void GetPhongNormal( int facenum, const FourVectors &spot, FourVectors &phongnormal )
{
        int j;
        dface_t *f = g_bspdata->dfaces + facenum;
        LVector3 facenormal;
        FourVectors vspot;

        const dplane_t *pl = getPlaneFromFace( f );

        VectorCopy( pl->normal, facenormal );
        phongnormal.DuplicateVector( facenormal );

        FourVectors face_centroid;
        face_centroid.DuplicateVector( g_face_centroids[facenum] );

        if ( g_smoothing_threshold != 1 )
        {

                faceneighbor_t *fn = &faceneighbor[facenum];

                // Calculate modified point normal for surface
                // Use the edge normals iff they are defined.  Bend the surface towards the edge normal(s)
                // Crude first attempt: find nearest edge normal and do a simple interpolation with facenormal.
                // Second attempt: find edge points+center that bound the point and do a three-point triangulation(baricentric)
                // Better third attempt: generate the point normals for all vertices and do baricentric triangulation.

                for ( j = 0; j < f->numedges; j++ )
                {
                        LVector3 v1, v2;
                        fltx4 a1, a2;
                        float aa, bb, ab;
                        int vert1, vert2;

                        LVector3 n1 = fn->normal[j];
                        LVector3 n2 = fn->normal[( j + 1 ) % f->numedges];

                        vert1 = EdgeVertex( f, j );
                        vert2 = EdgeVertex( f, j + 1 );

                        LVector3 p1 = GetLVector3_2( g_bspdata->dvertexes[vert1].point );
                        LVector3 p2 = GetLVector3_2( g_bspdata->dvertexes[vert2].point );

                        // Build vectors from the middle of the face to the edge vertexes and the sample pos.
                        VectorSubtract( p1, g_face_centroids[facenum], v1 );
                        VectorSubtract( p2, g_face_centroids[facenum], v2 );
                        vspot = spot;
                        vspot -= face_centroid;
                        aa = v1.dot( v1 );
                        bb = v2.dot( v2 );
                        ab = v1.dot( v2 );
                        a1 = ReciprocalSIMD( ReplicateX4( aa * bb - ab * ab ) );
                        a1 = MulSIMD( a1, SubSIMD( MulSIMD( ReplicateX4( bb ), vspot * v1 ), MulSIMD( ReplicateX4( ab ), vspot * v2 ) ) );
                        a2 = ReciprocalSIMD( ReplicateX4( bb ) );
                        a2 = MulSIMD( a2, SubSIMD( vspot *v2, MulSIMD( a1, ReplicateX4( ab ) ) ) );

                        fltx4 result_mask = AndSIMD( CmpGeSIMD( a1, Four_Zeros ), CmpGeSIMD( a2, Four_Zeros ) );
                        if ( !TestSignSIMD( result_mask ) )
                                continue;

                        // store the old phong normal to avoid overwriting the already computed phong normals
                        FourVectors oldphongnorm = FourVectors( phongnormal );

                        // calculate distance from edge to pos
                        FourVectors temp;
                        fltx4 scale;

                        // interpolate between the center and edge normals based on sample position
                        scale = SubSIMD( SubSIMD( Four_Ones, a1 ), a2 );
                        phongnormal.DuplicateVector( fn->facenormal );
                        phongnormal *= scale;
                        temp.DuplicateVector( n1 );
                        temp *= a1;
                        phongnormal += temp;
                        temp.DuplicateVector( n2 );
                        temp *= a2;
                        phongnormal += temp;

                        // restore the old phong normals
                        phongnormal.x = AddSIMD( AndSIMD( result_mask, phongnormal.x ), AndNotSIMD( result_mask, oldphongnorm.x ) );
                        phongnormal.y = AddSIMD( AndSIMD( result_mask, phongnormal.y ), AndNotSIMD( result_mask, oldphongnorm.y ) );
                        phongnormal.z = AddSIMD( AndSIMD( result_mask, phongnormal.z ), AndNotSIMD( result_mask, oldphongnorm.z ) );
                }

                phongnormal.VectorNormalize();
        }
}

void ComputeIlluminationPointAndNormalsSSE( const lightinfo_t &l, const FourVectors &pos,
                                            const FourVectors &norm, SSE_SampleInfo_t *info, int num_samples )
{
        LVector3 v[4];

        info->points = pos;
        bool comp_normals = ( info->normal_count > 1 && !l.isflat );

        // FIXME: move sample point off the surface a bit, this is done so that
        // light sampling will not be affected by a bug	where raycasts will
        // intersect with the face being lit. We really should just have that
        // logic in GatherSampleLight
        FourVectors facenormal;
        facenormal.DuplicateVector( l.facenormal );
        info->points += facenormal;

        if ( !l.isflat )
        {
                // TODO org models
                //FourVectors modelorg;
                //modelorg.DuplicateVector(l.model)

                FourVectors sample4 = pos;
                GetPhongNormal( info->facenum, sample4, info->point_normals[0] );
        }

        if ( comp_normals )
        {
                LVector3 bv[4][NUM_BUMP_VECTS];
                for ( int i = 0; i < 4; i++ )
                {
                        GetBumpNormals( info->texinfo, GetLVector3( l.facenormal ), info->point_normals[0].Vec( i ), bv[i] );
                }
                for ( int b = 0; b < NUM_BUMP_VECTS; b++ )
                {
                        info->point_normals[b + 1].LoadAndSwizzle( bv[0][b], bv[1][b], bv[2][b], bv[3][b] );
                }
        }

        for ( int i = 0; i < 4; i++ )
        {
                float vpos[3];
                VectorCopy( pos.Vec( i ), vpos );
                info->clusters[i] = PointInLeaf( vpos ) - g_bspdata->dleafs;
        }

}

#define NORMALFORMFACTOR	40.156979 // accumuated dot products for hemisphere

#define CONSTANT_DOT (.7/2)

#define NSAMPLES_SUN_AREA_LIGHT 30							// number of samples to take for an
                                                            // non-point sun light

/**
 * Gathers light from sun (emit_skylight)
 */
void GatherSampleSkyLightSSE( SSE_sampleLightInput_t &input, SSE_sampleLightOutput_t &out )
{
        bool ignore_normals = false;//lightflags & light_flag_t
        bool force_fast = false;// todo

        fltx4 dot;

        if ( ignore_normals )
                dot = ReplicateX4( CONSTANT_DOT );
        else
                dot = NegSIMD( input.normals[0] * input.dl->normal );

        dot = MaxSIMD( dot, Four_Zeros );
        int zero_mask = TestSignSIMD( CmpEqSIMD( dot, Four_Zeros ) );
        if ( zero_mask == 0xF )
                return;

        int nsamples = 1;
        if ( Lights::sun_angular_extent > 0.0 )
        {
                nsamples = NSAMPLES_SUN_AREA_LIGHT;
                if ( g_fastmode || force_fast )
                        nsamples /= 4;
        }

        fltx4 total_frac_vis = Four_Zeros;
        fltx4 frac_vis = Four_Zeros;

        DirectionalSampler_t sampler;

        for ( int d = 0; d < nsamples; d++ )
        {
                // determine visibility of skylight
                // search back to see if we can hit a sky brush
                LVector3 delta;
                VectorScale( input.dl->normal, -MAX_TRACE_LENGTH, delta );
                if ( d )
                {
                        // jitter light source location
                        LVector3 ofs = sampler.NextValue();
                        ofs *= MAX_TRACE_LENGTH * Lights::sun_angular_extent;
                        delta += ofs;
                }
                

                FourVectors delta4;
                delta4.DuplicateVector( delta );
                delta4 += input.pos;

                fltx4 this_fraction;
                RADTrace::test_four_lines( input.pos, delta4, &this_fraction, CONTENTS_SKY, true );

                total_frac_vis = AddSIMD( total_frac_vis, this_fraction );
        }

        fltx4 see_amount = MulSIMD( total_frac_vis, ReplicateX4( 1.0f / nsamples ) );
        out.dot[0] = MulSIMD( dot, see_amount );
        out.falloff = Four_Ones;
        out.sun_amount = MulSIMD( see_amount, ReplicateX4( 10000.0f ) );
        for ( int i = 1; i < input.normal_count; i++ )
        {
                if ( ignore_normals )
                        out.dot[i] = ReplicateX4( CONSTANT_DOT );
                else
                {
                        out.dot[i] = NegSIMD( input.normals[i] * input.dl->normal );
                        out.dot[i] = MulSIMD( out.dot[i], see_amount );
                }
        }
}

/**
 * Gathers light from ambient sky light (emit_skyambient)
 */
void GatherSampleAmbientSkySSE( SSE_sampleLightInput_t &input, SSE_sampleLightOutput_t &out )
{
        bool ignore_normals = false;//lightflags & light_flag_t todo
        bool force_fast = false;// todo

        fltx4 sumdot = Four_Zeros;
        fltx4 ambient_intensity[NUM_BUMP_VECTS + 1];
        fltx4 possible_hit_count[NUM_BUMP_VECTS + 1];
        fltx4 dots[NUM_BUMP_VECTS + 1];

        for ( int i = 0; i < input.normal_count; i++ )
        {
                ambient_intensity[i] = Four_Zeros;
                possible_hit_count[i] = Four_Zeros;
        }

        DirectionalSampler_t sampler;
        int sky_samples = NUMVERTEXNORMALS;
        if ( g_fastmode || force_fast )
                sky_samples /= 4;
        else
                sky_samples *= g_skysamplescale;

        for ( int j = 0; j < sky_samples; j++ )
        {
                FourVectors anorm;
                anorm.DuplicateVector( sampler.NextValue() );
                if ( ignore_normals )
                        dots[0] = ReplicateX4( CONSTANT_DOT );
                else
                        dots[0] = NegSIMD( input.normals[0] * anorm );

                fltx4 validity = CmpGtSIMD( dots[0], ReplicateX4( EQUAL_EPSILON ) );

                // no possibility of anybody getting lit
                if ( !TestSignSIMD( validity ) )
                        continue;

                dots[0] = AndSIMD( validity, dots[0] );
                sumdot = AddSIMD( dots[0], sumdot );
                possible_hit_count[0] = AddSIMD( AndSIMD( validity, Four_Ones ), possible_hit_count[0] );

                for ( int i = 1; i < input.normal_count; i++ )
                {
                        if ( ignore_normals )
                                dots[i] = ReplicateX4( CONSTANT_DOT );
                        else
                                dots[i] = NegSIMD( input.normals[i] * anorm );
                        fltx4 validity2 = CmpGtSIMD( dots[i], ReplicateX4( EQUAL_EPSILON ) );
                        dots[i] = AndSIMD( validity2, dots[i] );
                        possible_hit_count[i] = AddSIMD(
                                AndSIMD( AndSIMD( validity, validity2 ), Four_Ones ),
                                possible_hit_count[i] );
                }

                // search back to see if we can hit a sky brush
                FourVectors delta = anorm;
                delta *= -MAX_TRACE_LENGTH;
                delta += input.pos;
                FourVectors surface_pos = input.pos;
                FourVectors offset = anorm;
                offset *= -input.epsilon;
                surface_pos -= offset;

                fltx4 fraction_visible4;
                RADTrace::test_four_lines( surface_pos, delta, &fraction_visible4, CONTENTS_SKY, true );
                for ( int i = 0; i < input.normal_count; i++ )
                {
                        fltx4 added_amt = MulSIMD( fraction_visible4, dots[i] );
                        ambient_intensity[i] = AddSIMD( ambient_intensity[i], added_amt );
                }
        }

        out.falloff = Four_Ones;
        for ( int i = 0; i < input.normal_count; i++ )
        {
                // now scale out the missing parts of the hemisphere of this bump basis vector
                fltx4 factor = ReciprocalSIMD( possible_hit_count[0] );
                factor = MulSIMD( factor, possible_hit_count[i] );
                out.dot[i] = MulSIMD( factor, sumdot );
                out.dot[i] = ReciprocalSIMD( out.dot[i] );
                out.dot[i] = MulSIMD( ambient_intensity[i], out.dot[i] );
        }
}

void GatherSampleLightStandardSSE( SSE_sampleLightInput_t &input, SSE_sampleLightOutput_t &out )
{
        bool ignore_normals = false; // todo

        FourVectors src;
        src.DuplicateVector( vec3_origin );

        if ( input.dl->facenum == -1 )
        {
                src.DuplicateVector( input.dl->origin );
        }
        
        // Find light vector
        FourVectors delta;
        delta = src;
        delta -= input.pos;
        fltx4 dist2 = delta.length2();
        fltx4 rpcdist = ReciprocalSqrtSIMD( dist2 );
        delta *= rpcdist;
        fltx4 dist = SqrtEstSIMD( dist2 );

        // Compute dot
        fltx4 dot = ReplicateX4( (float)CONSTANT_DOT );
        if ( !ignore_normals )
                dot = delta * input.normals[0];
        dot = MaxSIMD( Four_Zeros, dot );

        // Affix dot to zero if past fade distz
        bool has_hard_falloff = ( input.dl->end_fade_distance > input.dl->start_fade_distance );
        if ( has_hard_falloff )
        {
                fltx4 notpastfadedist = CmpLeSIMD( dist, ReplicateX4( input.dl->end_fade_distance ) );
                dot = AndSIMD( dot, notpastfadedist );
                if ( !TestSignSIMD( notpastfadedist ) )
                        return;
        }

        dist = MaxSIMD( dist, Four_Ones );
        fltx4 falloffevaldist = MinSIMD( dist, ReplicateX4( input.dl->cap_distance ) );

        fltx4 constant, linear, quadratic;
        fltx4 dot2, incone, infringe, mult;
        FourVectors offset;

        switch ( input.dl->type )
        {
        case emit_point:
                constant = ReplicateX4( input.dl->constant_atten );
                linear = ReplicateX4( input.dl->linear_atten );
                quadratic = ReplicateX4( input.dl->quadratic_atten );

                out.falloff = MulSIMD( falloffevaldist, falloffevaldist );
                out.falloff = MulSIMD( out.falloff, quadratic );
                out.falloff = AddSIMD( out.falloff, MulSIMD( linear, falloffevaldist ) );
                out.falloff = AddSIMD( out.falloff, constant );
                out.falloff = ReciprocalSIMD( out.falloff );
                break;

        case emit_surface:
                dot2 = delta * input.dl->normal;
                dot2 = NegSIMD( dot2 );
                // light behind surface yields zero dot
                dot2 = MaxSIMD( Four_Zeros, dot2 );
                if ( TestSignSIMD( CmpEqSIMD( Four_Zeros, dot ) ) == 0xF )
                        return;

                out.falloff = ReciprocalSIMD( dist2 );
                out.falloff = MulSIMD( out.falloff, dot2 );

                // move the endpoint away from the surface by epsilon to prevent hittng the surface with trace
                offset.DuplicateVector( input.dl->normal );
                offset *= DIST_EPSILON;
                src += offset;
                break;

        case emit_spotlight:
                dot2 = delta * input.dl->normal;
                dot2 = NegSIMD( dot2 );

                // affix dot2 to zero if outside light cone
                incone = CmpGtSIMD( dot2, ReplicateX4( input.dl->stopdot2 ) );
                if ( !TestSignSIMD( incone ) )
                        return;
                dot = AndSIMD( incone, dot );

                constant = ReplicateX4( input.dl->constant_atten );
                linear = ReplicateX4( input.dl->linear_atten );
                quadratic = ReplicateX4( input.dl->quadratic_atten );

                out.falloff = MulSIMD( falloffevaldist, falloffevaldist );
                out.falloff = MulSIMD( out.falloff, quadratic );
                out.falloff = AddSIMD( out.falloff, MulSIMD( linear, falloffevaldist ) );
                out.falloff = AddSIMD( out.falloff, constant );
                out.falloff = ReciprocalSIMD( out.falloff );
                out.falloff = MulSIMD( out.falloff, dot2 );

                // outside the inner cone
                infringe = CmpLeSIMD( dot2, ReplicateX4( input.dl->stopdot ) );
                mult = ReplicateX4( input.dl->stopdot - input.dl->stopdot2 );
                mult = ReciprocalSIMD( mult );
                mult = MulSIMD( mult, SubSIMD( dot2, ReplicateX4( input.dl->stopdot2 ) ) );
                mult = MinSIMD( mult, Four_Ones );
                mult = MaxSIMD( mult, Four_Zeros );

                // pow is fixed point, so this isn't the most accurate, but it doesn't need to be
                if ( input.dl->exponent != 0.0 && input.dl->exponent != 1.0 )
                        mult = PowSIMD( mult, input.dl->exponent );

                mult = AndSIMD( infringe, mult );
                mult = AddSIMD( mult, AndNotSIMD( infringe, Four_Ones ) );
                out.falloff = MulSIMD( mult, out.falloff );
                break;
        }

        // we may be in the fade region - modulate lighting by the fade curve
        if ( has_hard_falloff )
        {
                fltx4 t = ReplicateX4( input.dl->end_fade_distance - input.dl->start_fade_distance );
                t = ReciprocalSIMD( t );
                t = MulSIMD( t, SubSIMD( dist, ReplicateX4( input.dl->start_fade_distance ) ) );

                // clamp t to [0...1]
                t = MinSIMD( t, Four_Ones );
                t = MaxSIMD( t, Four_Zeros );
                t = SubSIMD( Four_Ones, t );

                // Using QuinticInterpolatingPolynomial, SSE-ified
                // t * t * t *( t * ( t* 6.0 - 15.0 ) + 10.0 )
                mult = SubSIMD( MulSIMD( ReplicateX4( 6.0f ), t ), ReplicateX4( 15.0f ) );
                mult = AddSIMD( MulSIMD( mult, t ), ReplicateX4( 10.0f ) );
                mult = MulSIMD( MulSIMD( t, t ), mult );
                mult = MulSIMD( t, mult );
                out.falloff = MulSIMD( mult, out.falloff );
        }

        // ray trace for visibility
        fltx4 fraction_visible4;
        RADTrace::test_four_lines( input.pos, src, &fraction_visible4, CONTENTS_EMPTY, true );
        dot = MulSIMD( fraction_visible4, dot );
        out.dot[0] = dot;

        for ( int i = 1; i < input.normal_count; i++ )
        {
                if ( ignore_normals )
                        out.dot[i] = ReplicateX4( (float)CONSTANT_DOT );
                else
                {
                        out.dot[i] = input.normals[i] * delta;
                        out.dot[i] = MaxSIMD( Four_Zeros, out.dot[i] );
                }
        }
}

void GatherSampleLightSSE( SSE_sampleLightOutput_t &out, directlight_t *dl, int facenum,
                           const FourVectors &pos, FourVectors *normals, int normal_count,
                           int thread, int lightflags, float epsilon )
{
        for ( int b = 0; b < normal_count; b++ )
        {
                out.dot[b] = Four_Zeros;
        }

        out.falloff = Four_Zeros;
        out.sun_amount = Four_Zeros;
        nassertv( normal_count <= ( NUM_BUMP_VECTS + 1 ) );

        SSE_sampleLightInput_t inp;
        inp.dl = dl;
        inp.facenum = facenum;
        inp.pos = pos;
        inp.normals = normals;
        inp.normal_count = normal_count;
        inp.thread = thread;
        inp.lightflags = lightflags;
        inp.epsilon = epsilon;

        // skylights work fundamentally different than normal lights
        switch ( dl->type )
        {
        case emit_skylight:
                GatherSampleSkyLightSSE( inp, out );
                break;
        case emit_skyambient:
                GatherSampleAmbientSkySSE( inp, out );
                break;
        case emit_point:
        case emit_surface:
        case emit_spotlight:
                GatherSampleLightStandardSSE( inp, out );
                break;
        default:
                Error( "Bad dl->type %i in GatherSampleLightSSE", dl->type );
                break;
        }

        // NOTE: Notice here that if the light is on the back side of the face
        // (tested by checking the dot product of the face normal and the light position)
        // we don't want it to contribute to *any* of the bumped lightmaps. It glows
        // in disturbing ways if we don't do this.
        out.dot[0] = MaxSIMD( out.dot[0], Four_Zeros );
        fltx4 notZero = CmpGtSIMD( out.dot[0], Four_Zeros );
        for ( int n = 1; n < normal_count; n++ )
        {
                out.dot[n] = MaxSIMD( out.dot[n], Four_Zeros );
                out.dot[n] = AndSIMD( out.dot[n], notZero );
        }

}

static int FindOrAllocateLightstyleSamples( dface_t *f, facelight_t *fl, int style, int normals )
{
        // Search the lightstyles associated with the face for a match
        int k;
        for ( k = 0; k < MAXLIGHTMAPS; k++ )
        {
                if ( f->styles[k] == style )
                        break;

                // Found an empty entry, we can use it for a new lightstyle
                if ( f->styles[k] == 255 )
                {
                        AllocateLightstyleSamples( fl, k, normals );
                        f->styles[k] = style;
                        break;
                }
        }

        // check for overflow
        if ( k >= MAXLIGHTMAPS )
                return -1;

        return k;
}

void GatherSampleLightAt4Points( SSE_SampleInfo_t &info, int sample_idx, int num_samples )
{
        SSE_sampleLightOutput_t out;

        // iterate over all direct lights and add them to the particular sapmle
        for ( directlight_t *dl = Lights::activelights; dl != nullptr; dl = dl->next )
        {
                // is this light in the pvs?
                fltx4 dot_mask = Four_Zeros;
                bool skip = true;
                for ( int s = 0; s < num_samples; s++ )
                {
                        if ( PVSCheck( dl->pvs, info.clusters[s] ) )
                        {
                                dot_mask = SetComponentSIMD( dot_mask, s, 1.0f );
                                skip = false;
                        }
                }

                if ( skip )
                {
                        continue;
                }
                        

                GatherSampleLightSSE( out, dl, info.facenum, info.points,
                                      info.point_normals, info.normal_count, info.thread );

                // Apply the pvs check filter and compute falloff X dot
                fltx4 fxdot[NUM_BUMP_VECTS + 1];
                skip = true;
                for ( int b = 0; b < info.normal_count; b++ )
                {
                        fxdot[b] = MulSIMD( out.dot[b], dot_mask );
                        fxdot[b] = MulSIMD( fxdot[b], out.falloff );
                        if ( !IsAllZeros( fxdot[b] ) )
                                skip = false;
                }

                if ( skip )
                {
                        continue;
                }
                        

                // figure out the lightstyle for this particular sample
                int lightstyleidx = FindOrAllocateLightstyleSamples( info.face, info.facelight, dl->style, info.normal_count );
                if ( lightstyleidx < 0 )
                {
                        Warning( "Too many lightstyles on face %d", info.facenum );
                        continue;
                }

                bumpsample_t *samples = info.facelight->light[lightstyleidx];
		bumpsample_t *sunsamples = info.facelight->sunlight[lightstyleidx];
                for ( int n = 0; n < info.normal_count; n++ )
                {
                        for ( int i = 0; i < num_samples; i++ )
                        {
                                // record the lighting contribution for this sample

				// Store sunlight separately
				lightvalue_t *light;
				if ( dl->type == emit_skylight )
					light = &sunsamples[sample_idx + i].light[n];
				else
					light = &samples[sample_idx + i].light[n];

				light->AddLight( SubFloat( fxdot[n], i ),
					dl->intensity,
					SubFloat( out.sun_amount, i ) );
                        }
                }
        }
}

void ComputeLuxelIntensity( SSE_SampleInfo_t &info, int sampleidx, bumpsample_t *samples, float *sample_intensity )
{
        // Compute a separate intensity for each
        sample_t& sample = info.facelight->sample[sampleidx];
        int destIdx = sample.s + sample.t * info.lightmap_width;
        for ( int n = 0; n < info.normal_count; ++n )
        {
                float intensity = samples[sampleidx].light[n].Intensity();

                // convert to a linear perception space
                sample_intensity[n * info.lightmap_size + destIdx] = std::pow( intensity / 256.0, 1.0 / 2.2 );
        }
}

void ComputeSampleIntensities( SSE_SampleInfo_t &info, bumpsample_t *samples, float *sample_intensity )
{
        for ( int i = 0; i < info.facelight->numsamples; i++ )
        {
                ComputeLuxelIntensity( info, i, samples, sample_intensity );
        }
}

void ComputeLightmapGradients( SSE_SampleInfo_t &info, bool *has_processed_sample,
                               float *sample_intensity, float *gradient )
{
        int w = info.lightmap_width;
        int h = info.lightmap_height;

        facelight_t *fl = info.facelight;

        for ( int i = 0; i < fl->numsamples; i++ )
        {
                // don't supersample more than once
                if ( has_processed_sample[i] )
                        continue;

                gradient[i] = 0.0f;
                sample_t &sample = fl->sample[i];

                // Choose the maximum gradient of all bumped lightmap intensities
                for ( int n = 0; n < info.normal_count; ++n )
                {
                        int j = n * info.lightmap_size + sample.s + sample.t * w;

                        if ( sample.t > 0 )
                        {
                                if ( sample.s > 0 )
                                {
                                        gradient[i] = std::max( gradient[i], fabs( sample_intensity[j] - sample_intensity[j - 1 - w] ) );
                                }
                                        
                                gradient[i] = std::max( gradient[i], fabs( sample_intensity[j] - sample_intensity[j - w] ) );
                                if ( sample.s < w - 1 )
                                {
                                        gradient[i] = std::max( gradient[i], fabs( sample_intensity[j] - sample_intensity[j + 1 - w] ) );
                                }
                                        
                        }
                        if ( sample.t < h - 1 )
                        {
                                if ( sample.s > 0 )
                                {
                                        gradient[i] = std::max( gradient[i], fabs( sample_intensity[j] - sample_intensity[j - 1 + w] ) );
                                }
                                        
                                gradient[i] = std::max( gradient[i], fabs( sample_intensity[j] - sample_intensity[j + w] ) );
                                if ( sample.s < w - 1 )
                                {
                                        gradient[i] = std::max( gradient[i], fabs( sample_intensity[j] - sample_intensity[j + 1 + w] ) );
                                }
                                        
                        }
                        if ( sample.s > 0 )
                        {
                                gradient[i] = std::max( gradient[i], fabs( sample_intensity[j] - sample_intensity[j - 1] ) );
                        }
                                
                        if ( sample.s < w - 1 )
                        {
                                gradient[i] = std::max( gradient[i], fabs( sample_intensity[j] - sample_intensity[j + 1] ) );
                        }
                }
        }
}

bool PointsInWinding( FourVectors const & point, Winding *w, int &invalidBits )
{
        FourVectors edge, toPt, cross, testCross, p0, p1;
        fltx4 invalidMask;

        //
        // get the first normal to test
        //
        p0.DuplicateVector( w->m_Points[0] );
        p1.DuplicateVector( w->m_Points[1] );
        toPt = point;
        toPt -= p0;
        edge = p1;
        edge -= p0;
        testCross = edge ^ toPt;
        testCross.VectorNormalizeFast();

        for ( int ndxPt = 1; ndxPt < w->m_NumPoints; ndxPt++ )
        {
                p0.DuplicateVector( w->m_Points[ndxPt] );
                p1.DuplicateVector( w->m_Points[( ndxPt + 1 ) % w->m_NumPoints] );
                toPt = point;
                toPt -= p0;
                edge = p1;
                edge -= p0;
                cross = edge ^ toPt;
                cross.VectorNormalizeFast();

                fltx4 dot = cross * testCross;
                invalidMask = OrSIMD( invalidMask, CmpLtSIMD( dot, Four_Zeros ) );

                invalidBits = TestSignSIMD( invalidMask );
                if ( invalidBits == 0xF )
                        return false;
        }

        return true;
}

void ResampleLightAt4Points( SSE_SampleInfo_t &info, int style, bool ambient, bumpsample_t *result)//, bumpsample_t *sunresult )
{
        SSE_sampleLightOutput_t out;

        // clear result
        for ( int i = 0; i < 4; i++ )
        {
                for ( int n = 0; n < info.normal_count; n++ )
                {
                        result[i].light[n].Zero();
                }
        }

        // Iterate over all direct lights and add them to the particular sample
        for ( directlight_t *dl = Lights::activelights; dl != nullptr; dl = dl->next )
        {
                if ( ambient && dl->type != emit_skyambient )
                        continue;
                if ( !ambient && dl->type == emit_skyambient )
                        continue;

		if ( dl->type == emit_skylight )
			continue;

                // only add contributions that match the lightstyle
                nassertv( style <= MAXLIGHTMAPS );
                nassertv( info.face->styles[style] != 0xFF );
                if ( dl->style != info.face->styles[style] )
                        continue;

                // is this light potentially visible?
                fltx4 dot_mask = Four_Zeros;
                bool skip = true;
                for ( int s = 0; s < 4; s++ )
                {
                        if ( PVSCheck( dl->pvs, info.clusters[s] ) )
                        {
                                dot_mask = SetComponentSIMD( dot_mask, s, 1.0f );
                                skip = false;
                        }
                }

                if ( skip )
                        // light not potentially visible
                        continue;

                // NOTE: Notice here that if the light is on the back side of the face
                // (tested by checking the dot product of the face normal and the light position)
                // we don't want it to contribute to *any* of the bumped lightmaps. It glows
                // in disturbing ways if we don't do this.
                GatherSampleLightSSE( out, dl, info.facenum, info.points, info.point_normals, info.normal_count, info.thread );

                // Apply the PVS check filter and compute falloff x dot
                fltx4 fxdot[NUM_BUMP_VECTS + 1];
                for ( int b = 0; b < info.normal_count; b++ )
                {
                        fxdot[b] = MulSIMD( out.falloff, out.dot[b] );
                        fxdot[b] = MulSIMD( fxdot[b], dot_mask );
                }

                // Compute the contributions to each of the bumped lightmaps
                // The first sample is for non-bumped lighting.
                // The other sample are for bumpmapping.
                for ( int i = 0; i < 4; ++i )
                {
                        for ( int n = 0; n < info.normal_count; ++n )
                        {
				bumpsample_t *pOut;
				//if ( dl->type == emit_skylight )
				//	pOut = sunresult;
				//else
					pOut = result;
                                pOut[i].light[n].AddLight( SubFloat( fxdot[n], i ), dl->intensity, SubFloat( out.sun_amount, i ) );
                        }
                }
        }
}

int SupersampleLightAtPoint( lightinfo_t &l, SSE_SampleInfo_t &info, int sampleidx,
                             int style, bumpsample_t &light, bool ambient )
{
        sample_t &sample = info.facelight->sample[sampleidx];

        // Get the position of the original sample in lightmap space
        LVector2 temp;
        WorldToLuxelSpace( &l, GetLVector3( sample.pos ), temp );
        LVector3 sample_light_origin( temp[0], temp[1], 0.0 );

        // some parameters related to supersampling
        float sample_width = !ambient ? 4 : 2;
        float cscale = 1.0 / sample_width;
        float csshift = -( ( sample_width - 1 ) * cscale ) / 2.0;

        // clear out the light values
        for ( int i = 0; i < info.normal_count; i++ )
        {
                light.light[i].Zero();
        }

        int subsamplecnt = 0;

        FourVectors supersamplenorm;
        supersamplenorm.DuplicateVector( sample.normal );

        FourVectors supersamplecoord;
        FourVectors supersamplepos;

        if ( !ambient )
        {
                float arow[4];
                for ( int coord = 0; coord < 4; coord++ )
                {
                        arow[coord] = csshift + coord * cscale;
                }
                fltx4 sserow = LoadUnalignedSIMD( arow );

                for ( int s = 0; s < 4; s++ )
                {
                        // make sure the coordinate is inside of the sample's winding and when normalizing
                        // below use the number of samples used, not just numsamples and some of them
                        // will be skipped if they are not inside of the winding
                        supersamplecoord.DuplicateVector( sample_light_origin );
                        supersamplecoord.x = AddSIMD( supersamplecoord.x, ReplicateX4( arow[s] ) );
                        supersamplecoord.y = AddSIMD( supersamplecoord.y, sserow );

                        // Figure out where the supersample exists in the world, and make sure
                        // it lies within the sample winding
                        LuxelToWorldSpace( &l, supersamplecoord[0], supersamplecoord[1], supersamplepos );

                        // A winding should exist only if the sample wasn't a uniform luxel, or if g_bDumpPatches is true.
                        int invalidBits = 0;
                        if ( sample.winding && !PointsInWinding( supersamplepos, sample.winding, invalidBits ) )
                                continue;

                        // Compute the super-sample illumination point and normal
                        // We're assuming the flat normal is the same for all supersamples
                        ComputeIlluminationPointAndNormalsSSE( l, supersamplepos, supersamplenorm, &info, 4 );

                        // Resample the non-ambient light at this point...
                        bumpsample_t result[4];
			//bumpsample_t sunresult[4];
			ResampleLightAt4Points( info, style, false, result );//, sunresult );

                        // got more subsamples
                        for ( int i = 0; i < 4; i++ )
                        {
                                if ( !( ( invalidBits >> i ) & 0x1 ) )
                                {
                                        for ( int n = 0; n < info.normal_count; ++n )
                                        {
                                                light[n].AddLight( result[i][n] );
						
                                        }
                                        ++subsamplecnt;
                                }
                        }
                }
        }
        else
        {
                FourVectors superSampleOffsets;
                superSampleOffsets.LoadAndSwizzle( LVector3( csshift, csshift, 0 ), LVector3( csshift, csshift + cscale, 0 ),
                                                   LVector3( csshift + cscale, csshift, 0 ), LVector3( csshift + cscale, csshift + cscale, 0 ) );
                supersamplecoord.DuplicateVector( sample_light_origin );
                supersamplecoord += superSampleOffsets;

                LuxelToWorldSpace( &l, supersamplecoord[0], supersamplecoord[1], supersamplepos );

                int invalidBits = 0;
                if ( sample.winding && !PointsInWinding( supersamplepos, sample.winding, invalidBits ) )
                        return 0;

                ComputeIlluminationPointAndNormalsSSE( l, supersamplepos, supersamplenorm, &info, 4 );

                bumpsample_t result[4];
                ResampleLightAt4Points( info, style, true, result );

                // Got more subsamples
                for ( int i = 0; i < 4; i++ )
                {
                        if ( !( ( invalidBits >> i ) & 0x1 ) )
                        {
                                for ( int n = 0; n < info.normal_count; ++n )
                                {
                                        light[n].AddLight( result[i][n] );
                                }
                                ++subsamplecnt;
                        }
                }
        }

        return subsamplecnt;
}

void BuildSupersampleFacelights( lightinfo_t &l, SSE_SampleInfo_t &info, int style )
{
        bumpsample_t ambientlight;
        bumpsample_t directlight;

        // this is used to make sure we don't supersample a particular sample more than once
        int processed_sample_size = info.lightmap_size * sizeof( bool );
        bool *has_processed_sample = (bool *)malloc( processed_sample_size );
        memset( has_processed_sample, 0, processed_sample_size );

        // this is used to compute a simple gradiant of the light samples
        // We're going to store the maximum intensity of all bumped samples at each sample location
        float *gradient = (float*)malloc( info.facelight->numsamples * sizeof( float ) );
        float *sample_intensity = (float *)malloc( info.normal_count * info.lightmap_size * sizeof( float ) );

        // compute the maximum intensity of all lighting associated with this lightstyle
        // for all bumped lighting
        bumpsample_t *lightsamples = info.facelight->light[style];
        ComputeSampleIntensities( info, lightsamples, sample_intensity );

        // What's going on here is that we're looking for large lighting discontinuities
        // (large light intensity gradients) as a clue that we should probably be supersampling
        // in that area. Because the supersampling operation will cause lighting changes,
        // we've found that it's good to re-check the gradients again and see if any other
        // areas should be supersampled as a result of the previous pass. Keep going
        // until all the gradients are reasonable or until we hit a max number of passes
        bool do_anotherpass = true;
        int pass = 1;
        while ( do_anotherpass && pass <= g_extrapasses )
        {
                // Look for lighting discontinuities to see what we should be supersampling
                ComputeLightmapGradients( info, has_processed_sample, sample_intensity, gradient );

                do_anotherpass = false;
                for ( int i = 0; i < info.facelight->numsamples; i++ )
                {
                        // don't supersample more than once
                        if ( has_processed_sample[i] )
                                continue;

                        // don't supersample if the lighting is pretty uniform near the sample
                        if ( gradient[i] < 0.0625 )
                                continue;

                        // Joy! We're supersampling now, and we therefore must do another pass
                        // Also, we need never bother with this sample again
                        has_processed_sample[i] = true;
                        do_anotherpass = true;

                        int ambient_supersample_count = SupersampleLightAtPoint( l, info, i, style, ambientlight, true );
                        int direct_supersample_count = SupersampleLightAtPoint( l, info, i, style, directlight, false );

                        if ( ambient_supersample_count > 0 && direct_supersample_count > 0 )
                        {
                                // add the ambient + directional terms together, stick it back into the lightmap
                                for ( int n = 0; n < info.normal_count; n++ )
                                {
                                        lightsamples[i][n].Zero();
                                        lightsamples[i][n].AddWeighted( directlight[n], 1.0 / direct_supersample_count );
                                        lightsamples[i][n].AddWeighted( ambientlight[n], 1.0 / ambient_supersample_count );
                                }

                                // Recompute the luxel intensity based on the supersampling
                                ComputeLuxelIntensity( info, i, lightsamples, sample_intensity );
                        }
                }

                // we've finished another supersampling pass
                pass++;
        }

        free( gradient );
        free( sample_intensity );
        free( has_processed_sample );
}

void AddSampleToPatch( sample_t *sample, lightvalue_t &light, int facenum )
{
        patch_t *patch;
        vec3_t mins, maxs;
        int i;

        if ( g_numbounce == 0 )
                return;
        if ( VectorAvg( light.light ) < 1 )
        {
                return;
        }    

        //
        // fixed the sample position and normal -- need to find the equiv pos, etc to set up
        // patches
        //
        if ( g_face_patches[facenum] == -1 )
                return;

        float radius = std::sqrt( sample->area ) / 2.0;

        patch_t *nextpatch = nullptr;
        for ( patch = &g_patches[g_face_patches[facenum]]; patch; patch = nextpatch )
        {
                // next patch
                nextpatch = nullptr;
                if ( patch->next != -1 )
                {
                        nextpatch = &g_patches[patch->next];
                }

                if ( patch->sky )
                        continue;

                // skip patches with children
                if ( patch->child1 != -1 )
                        continue;

                patch->winding->getBounds( mins, maxs );

                bool skip = false;
                for ( i = 0; i < 3; i++ )
                {
                        if ( ( mins[i] > sample->pos[i] + radius ) ||
                                ( maxs[i] < sample->pos[i] - radius ) )
                        {
                                skip = true;
                                break;
                        }

                }

                if ( skip )
                        continue;

                // add the sample to the patch
                patch->samplearea += sample->area;
                VectorMA( patch->samplelight, sample->area, light.light, patch->samplelight );
        }

        // don't worry if some samples don't find a patch
}

/**
 * Determines the brightness of each patch on a particular face
 * from the calculated lightmap for the radiosity (light bouncing) pass.
 */
void BuildPatchLights( int facenum )
{
        int i, k;
        patch_t *patch;
        dface_t *f = g_bspdata->dfaces + facenum;
        facelight_t *fl = &facelight[facenum];

        for ( k = 0; k < MAXLIGHTMAPS; k++ )
        {
                if ( f->styles[k] == 0 )
                        break;
        }

        if ( k >= MAXLIGHTMAPS )
                return;

        for ( i = 0; i < fl->numsamples; i++ )
        {
		lightvalue_t totalDirect = fl->light[k][i][0];
		totalDirect.AddLight( fl->sunlight[k][i][0] );

                AddSampleToPatch( &fl->sample[i], totalDirect, facenum );
        }

        // check for a valid face
        if ( g_face_patches[facenum] == -1 )
                return;

        // push up sampled light to parents (children always exist first in the list)
        patch_t *nextpatch;
        for ( patch = &g_patches[g_face_patches[facenum]]; patch; patch = nextpatch )
        {
                // next patch
                nextpatch = nullptr;
                if ( patch->next != -1 )
                {
                        nextpatch = &g_patches[patch->next];
                }

                // skip patches without parents
                if ( patch->parent == -1 )
                        continue;

                patch_t *parent = &g_patches[patch->parent];
                parent->samplearea += patch->samplearea;
                VectorAdd( parent->samplelight, patch->samplelight, parent->samplelight );
        }

        // average up the direct light on each patch for radiosity
        if ( g_numbounce > 0 )
        {
                for ( patch = &g_patches[g_face_patches[facenum]]; patch; patch = nextpatch )
                {
                        // next patch
                        nextpatch = nullptr;
                        if ( patch->next != -1 )
                        {
                                nextpatch = &g_patches[patch->next];
                        }

                        if ( patch->samplearea )
                        {
                                float scale;
                                LVector3 v;
                                scale = 1.0 / patch->samplearea;

                                v = patch->samplelight * scale;
                                patch->totallight.light[0].light += v;
                                patch->directlight += v;
                        }
                }
        }

        // pull totallight from all children (children always exist first in the list)
        for ( patch = &g_patches[g_face_patches[facenum]]; patch; patch = nextpatch )
        {
                // next patch
                nextpatch = nullptr;
                if ( patch->next != -1 )
                {
                        nextpatch = &g_patches[patch->next];
                }

                if ( patch->child1 != -1 )
                {
                        float s1, s2;
                        patch_t *child1, *child2;

                        child1 = &g_patches[patch->child1];
                        child2 = &g_patches[patch->child2];

                        s1 = child1->area / ( child1->area + child2->area );
                        s2 = child2->area / ( child1->area + child2->area );

                        child1->totallight.light[0].light *= s1;
                        VectorMA( patch->totallight.light[0].light, s2,
                                  child2->totallight.light[0].light,
                                  patch->totallight.light[0].light );

                        patch->totallight.light[0].light = patch->directlight;
                }
        }

        bool needsbump = fl->bumped;

        // add an ambient term if desired
#if 0
        if ( g_ambient[0] || g_ambient[1] || g_ambient[2] )
        {
                LVector3 ambient = GetLVector3( g_ambient );
                for ( int j = 0; j < MAXLIGHTMAPS && f->styles[j] != 255; j++ )
                {
                        if ( f->styles[j] == 0 )
                        {
                                for ( i = 0; i < fl->numsamples; i++ )
                                {
                                        fl->light[j][i][0].light += ambient;
                                        if ( needsbump )
                                        {
                                                fl->light[j][i][1].light += ambient;
                                                fl->light[j][i][2].light += ambient;
                                                fl->light[j][i][3].light += ambient;
                                        }
                                }
                                break;
                        }
                }
        }
#endif
}

void DetermineLightmapMemory()
{
        float lm_bytes = 0;
        float lm_megabytes = 0;

        lightinfo_t l;
        for ( int facenum = 0; facenum < g_bspdata->numfaces; facenum++ )
        {
                l.surfnum = facenum;
                l.face = g_bspdata->dfaces + facenum;
                CalcFaceExtents( &l );

                lm_bytes += ( l.face->lightmap_size[0] + 1 ) * ( l.face->lightmap_size[1] + 1 ) * sizeof( colorrgbexp32_t );
        }

        lm_megabytes = lm_bytes / 1e6;

        std::cout << "Lightmaps will take up " << lm_megabytes << " MB of storage" << std::endl;
}

/**
 * Calculates a lightmap for a particular face, then
 * uses the calculated lightmap to figure out brightness of each patch on the face.
 */
void BuildFacelights( const int facenum )
{
        
        dface_t*                f;
        lightinfo_t             l;
        int                     i;
        int                     j;
        LVector3                v[4], n[4];
        const dplane_t*         plane;
        SSE_SampleInfo_t        sampleinfo;

        f = &g_bspdata->dfaces[facenum];
        //
        // some surfaces don't need lightmaps
        //
        f->lightofs = -1;
        f->bouncedlightofs = -1;
        f->sunlightofs = -1;
        for ( j = 0; j < MAXLIGHTMAPS; j++ )
        {
                f->styles[j] = 255;
        }

        if ( g_bspdata->texinfo[f->texinfo].flags & TEX_SPECIAL )
        {
                return;                                            // non-lit texture
        }
        
        memset( &l, 0, sizeof( l ) );

        plane = getPlaneFromFace( f );

        facelight_t *fl = &facelight[facenum];

        InitLightInfo( l, facenum );
        CalcPoints( &l, fl, facenum );
        InitSampleInfo( l, GetCurrentThreadNumber(), sampleinfo );

        // allocate sample positions/normals to SSE
        int num_groups = sampleinfo.num_sample_groups;

        // always allocate style 0 lightmap
        f->styles[0] = 0;
        AllocateLightstyleSamples( fl, 0, sampleinfo.normal_count );

        // sample the lights at each sample location
        for ( int grp = 0; grp < num_groups; grp++ )
        {
                int nsample = 4 * grp;

                sample_t *sample = sampleinfo.facelight->sample + nsample;
                int num_samples = std::min( 4, sampleinfo.facelight->numsamples - nsample );

                FourVectors positions;
                FourVectors normals;
                
                for ( int i = 0; i < 4; i++ )
                {
                        v[i] = ( i < num_samples ) ? GetLVector3( sample[i].pos ) : GetLVector3( sample[num_samples - 1].pos );
                        n[i] = ( i < num_samples ) ? GetLVector3( sample[i].normal ) : GetLVector3( sample[num_samples - 1].normal );
                }

                positions.LoadAndSwizzle( v[0], v[1], v[2], v[3] );
                normals.LoadAndSwizzle( n[0], n[1], n[2], n[3] );

                ComputeIlluminationPointAndNormalsSSE( l, positions, normals, &sampleinfo, num_samples );

                if ( !l.isflat )
                {
                        // fixup sample normals in case of smooth faces
                        for ( int i = 0; i < num_samples; i++ )
                        {
                                VectorCopy( sampleinfo.point_normals[0].Vec( i ), sample[i].normal );
                        }
                }

                // iterate over all the lights and add their contribution to this group of spots
                GatherSampleLightAt4Points( sampleinfo, nsample, num_samples );
        }

        if ( g_extra )
        {
                // for each lightstyle, perform a supersampling pass
                for ( i = 0; i < MAXLIGHTMAPS; i++ )
                {
                        // stop when we run out of lightstyles
                        if ( f->styles[i] == 0xFF )
                                break;

                        BuildSupersampleFacelights( l, sampleinfo, i );
                }
        }

        BuildPatchLights( facenum );

        lightinfo[facenum] = l;
        
}

// =====================================================================================
//  PrecompLightmapOffsets
// =====================================================================================
void PrecompLightmapOffsets()
{
        int             facenum;
        dface_t*        f;
        facelight_t*    fl;
        int             lightstyles;

        g_bspdata->lightdata.clear();

        for ( facenum = 0; facenum < g_bspdata->numfaces; facenum++ )
        {
                f = &g_bspdata->dfaces[facenum];
                fl = &facelight[facenum];

                if ( g_bspdata->texinfo[f->texinfo].flags & TEX_SPECIAL )
                {
                        continue;                                      // non-lit texture
                }

                for ( lightstyles = 0; lightstyles < MAXLIGHTMAPS; lightstyles++ )
                {
                        if ( f->styles[lightstyles] == 255 )
                        {
                                break;
                        }
                }

                if ( !lightstyles )
                {
                        continue;
                }

                int luxels = ( f->lightmap_size[0] + 1 ) * ( f->lightmap_size[1] + 1 );

                // Direct lighting
                f->lightofs = g_bspdata->lightdata.size();
                for ( int i = 0; i < luxels * lightstyles * fl->normal_count; i++ )
                {
                        // Initialize all luxels for this face.
                        colorrgbexp32_t sample;
                        memset( &sample, 0, sizeof( colorrgbexp32_t ) );
                        g_bspdata->lightdata.push_back( sample );
                }

                // Bounced lighting
                f->bouncedlightofs = g_bspdata->bouncedlightdata.size();
                for ( int i = 0; i < luxels; i++ )
                {
                        colorrgbexp32_t sample;
                        memset( &sample, 0, sizeof( colorrgbexp32_t ) );
                        g_bspdata->bouncedlightdata.push_back( sample );
                }
        }
}

void ReduceLightmap()
{
        pvector<colorrgbexp32_t> oldlightdata = g_bspdata->lightdata;
        g_bspdata->lightdata.clear();

        int facenum;
        for ( facenum = 0; facenum < g_bspdata->numfaces; facenum++ )
        {
                dface_t *f = &g_bspdata->dfaces[facenum];
                facelight_t *fl = &facelight[facenum];
                if ( g_bspdata->texinfo[f->texinfo].flags & TEX_SPECIAL )
                {
                        continue;                                      // non-lit texture
                }
                // just need to zero the lightmap so that it won't contribute to lightdata size
                if ( IntForKey( g_face_entity[facenum], "zhlt_striprad" ) )
                {
                        f->lightofs = g_bspdata->lightdata.size();
                        for ( int k = 0; k < MAXLIGHTMAPS; k++ )
                        {
                                f->styles[k] = 255;
                        }
                        continue;
                }
                if ( f->lightofs == -1 )
                {
                        continue;
                }

                int i, k;
                int oldofs;
                unsigned char oldstyles[MAXLIGHTMAPS];
                oldofs = f->lightofs;
                f->lightofs = g_bspdata->lightdata.size();
                for ( k = 0; k < MAXLIGHTMAPS; k++ )
                {
                        oldstyles[k] = f->styles[k];
                        f->styles[k] = 255;
                }
                int numstyles = 0;
                for ( k = 0; k < MAXLIGHTMAPS && oldstyles[k] != 255; k++ )
                {
                        unsigned char maxb = 0;
                        for ( i = 0; i < fl->numsamples; i++ )
                        {
                                colorrgbexp32_t col = oldlightdata[oldofs + fl->numsamples * k + i];
                                LVector3 vcol( 0 );
                                ColorRGBExp32ToVector( col, vcol );
                                maxb = qmax( maxb, VectorMaximum( vcol ) );
                        }
                        if ( maxb <= 0 ) // black
                        {
                                continue;
                        }
                        f->styles[numstyles] = oldstyles[k];
                        hlassume( g_bspdata->lightdata.size() + fl->numsamples * ( numstyles + 1 ) <= g_max_map_lightdata, assume_MAX_MAP_LIGHTING );
                        for ( int j = 0; j < fl->numsamples; j++ )
                        {
                                g_bspdata->lightdata.push_back(oldlightdata[oldofs + fl->numsamples * k + j]);
                        }
                        numstyles++;
                }
        }
}

typedef struct facelightlist_s
{
        int facenum;
        facelightlist_s *next;
}
facelightlist_t;

static facelightlist_t *g_dependentfacelights[MAX_MAP_FACES];

// =====================================================================================
//  CreateFacelightDependencyList
// =====================================================================================
void CreateFacelightDependencyList()
{
        int facenum;
        dface_t *f;
        facelight_t *fl;
        int i;
        int k;
        int surface;
        facelightlist_t *item;

        for ( i = 0; i < MAX_MAP_FACES; i++ )
        {
                g_dependentfacelights[i] = NULL;
        }

        // for each face
        for ( facenum = 0; facenum < g_bspdata->numfaces; facenum++ )
        {
                f = &g_bspdata->dfaces[facenum];
                fl = &facelight[facenum];
                if ( g_bspdata->texinfo[f->texinfo].flags & TEX_SPECIAL )
                {
                        continue;
                }

                for ( k = 0; k < MAXLIGHTMAPS && f->styles[k] != 255; k++ )
                {
                        for ( i = 0; i < fl->numsamples; i++ )
                        {
                                surface = fl->samples[k][i].surface; // that surface contains at least one sample from this face
                                if ( 0 <= surface && surface < g_bspdata->numfaces )
                                {
                                        // insert this face into the dependency list of that surface
                                        for ( item = g_dependentfacelights[surface]; item != NULL; item = item->next )
                                        {
                                                if ( item->facenum == facenum )
                                                        break;
                                        }
                                        if ( item )
                                        {
                                                continue;
                                        }

                                        item = (facelightlist_t *)malloc( sizeof( facelightlist_t ) );
                                        hlassume( item != NULL, assume_NoMemory );
                                        item->facenum = facenum;
                                        item->next = g_dependentfacelights[surface];
                                        g_dependentfacelights[surface] = item;
                                }
                        }
                }
        }
}

// =====================================================================================
//  FreeFacelightDependencyList
// =====================================================================================
void FreeFacelightDependencyList()
{
        int i;
        facelightlist_t *item;

        for ( i = 0; i < MAX_MAP_FACES; i++ )
        {
                while ( g_dependentfacelights[i] )
                {
                        item = g_dependentfacelights[i];
                        g_dependentfacelights[i] = item->next;
                        free( item );
                }
        }
}

// =====================================================================================
//  ScaleDirectLights
// =====================================================================================
void ScaleDirectLights()
{
        int facenum;
        dface_t *f;
        facelight_t *fl;
        int i;
        int k;
        sample_t *samp;

        for ( facenum = 0; facenum < g_bspdata->numfaces; facenum++ )
        {
                f = &g_bspdata->dfaces[facenum];

                if ( g_bspdata->texinfo[f->texinfo].flags & TEX_SPECIAL )
                {
                        continue;
                }

                fl = &facelight[facenum];

                for ( int n = 0; n < fl->normal_count; n++ )
                {
                        for ( k = 0; k < MAXLIGHTMAPS && f->styles[k] != 255; k++ )
                        {
                                for ( i = 0; i < fl->numsamples; i++ )
                                {
                                        samp = &fl->samples[k][i];
                                        VectorScale( samp->light[n], g_direct_scale, samp->light[n] );
                                }
                        }
                }
                
        }
}

