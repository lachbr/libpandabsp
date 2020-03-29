#include "csg.h"

#include <algorithm>

plane_t         g_mapplanes[MAX_INTERNAL_MAP_PLANES];
int             g_nummapplanes;
hullshape_t		g_defaulthulls[NUM_HULLS];
int				g_numhullshapes;
hullshape_t		g_hullshapes[MAX_HULLSHAPES];

#define DIST_EPSILON   0.04


// =====================================================================================
//  FindIntPlane, fast version (replacement by KGP)
//	This process could be optimized by placing the planes in a (non hash-) set and using
//	half of the inner loop check below as the comparator; I'd expect the speed gain to be
//	very large given the change from O(N^2) to O(NlogN) to build the set of planes.
// =====================================================================================

int FindIntPlane( const vec_t* const normal, const vec_t* const origin )
{
        int             returnval;
        plane_t*        p;
        plane_t         temp;
        vec_t           t;

        returnval = 0;

find_plane:
        for ( ; returnval < g_nummapplanes; returnval++ )
        {
                // BUG: there might be some multithread issue --vluzacn
                if ( -DIR_EPSILON < ( t = normal[0] - g_mapplanes[returnval].normal[0] ) && t < DIR_EPSILON &&
                     -DIR_EPSILON < ( t = normal[1] - g_mapplanes[returnval].normal[1] ) && t < DIR_EPSILON &&
                     -DIR_EPSILON < ( t = normal[2] - g_mapplanes[returnval].normal[2] ) && t < DIR_EPSILON )
                {
                        t = DotProduct( origin, g_mapplanes[returnval].normal ) - g_mapplanes[returnval].dist;

                        if ( -DIST_EPSILON < t && t < DIST_EPSILON )
                        {
                                return returnval;
                        }
                }
        }

        ThreadLock();
        if ( returnval != g_nummapplanes ) // make sure we don't race
        {
                ThreadUnlock();
                goto find_plane; //check to see if other thread added plane we need
        }

        // create new planes - double check that we have room for 2 planes
        hlassume( g_nummapplanes + 1 < MAX_INTERNAL_MAP_PLANES, assume_MAX_INTERNAL_MAP_PLANES );

        p = &g_mapplanes[g_nummapplanes];

        VectorCopy( origin, p->origin );
        VectorCopy( normal, p->normal );
        VectorNormalize( p->normal );
        p->type = PlaneTypeForNormal( p->normal );
        if ( p->type <= last_axial )
        {
                for ( int i = 0; i < 3; i++ )
                {
                        if ( i == p->type )
                                p->normal[i] = p->normal[i] > 0 ? 1 : -1;
                        else
                                p->normal[i] = 0;
                }
        }
        p->dist = DotProduct( origin, p->normal );

        VectorCopy( origin, ( p + 1 )->origin );
        VectorSubtract( vec3_origin, p->normal, ( p + 1 )->normal );
        ( p + 1 )->type = p->type;
        ( p + 1 )->dist = -p->dist;

        // always put axial planes facing positive first
        if ( normal[( p->type ) % 3] < 0 )
        {
                temp = *p;
                *p = *( p + 1 );
                *( p + 1 ) = temp;
                returnval = g_nummapplanes + 1;
        }
        else
        {
                returnval = g_nummapplanes;
        }

        g_nummapplanes += 2;
        ThreadUnlock();
        return returnval;
}



int PlaneFromPoints( const vec_t* const p0, const vec_t* const p1, const vec_t* const p2 )
{
        vec3_t          v1, v2;
        vec3_t          normal;

        VectorSubtract( p0, p1, v1 );
        VectorSubtract( p2, p1, v2 );
        CrossProduct( v1, v2, normal );
        if ( VectorNormalize( normal ) )
        {
                return FindIntPlane( normal, p0 );
        }
        return -1;
}

// =====================================================================================
//  MakeHullFaces
// =====================================================================================
void SortSides( brushhull_t *h )
{
        int numsides;
        bface_t **sides;
        vec3_t *normals;
        bool *isused;
        int i, j;
        int *sorted;
        bface_t *f;
        for ( numsides = 0, f = h->faces; f; f = f->next )
        {
                numsides++;
        }
        sides = (bface_t **)malloc( numsides * sizeof( bface_t * ) );
        hlassume( sides != NULL, assume_NoMemory );
        normals = (vec3_t *)malloc( numsides * sizeof( vec3_t ) );
        hlassume( normals != NULL, assume_NoMemory );
        isused = (bool *)malloc( numsides * sizeof( bool ) );
        hlassume( isused != NULL, assume_NoMemory );
        sorted = (int *)malloc( numsides * sizeof( int ) );
        hlassume( sorted != NULL, assume_NoMemory );
        for ( i = 0, f = h->faces; f; i++, f = f->next )
        {
                sides[i] = f;
                isused[i] = false;
                const plane_t *p = &g_mapplanes[f->planenum];
                VectorCopy( p->normal, normals[i] );
        }
        for ( i = 0; i < numsides; i++ )
        {
                int bestside;
                int bestaxial = -1;
                for ( j = 0; j < numsides; j++ )
                {
                        if ( isused[j] )
                        {
                                continue;
                        }
                        int axial = ( fabs( normals[j][0] ) < NORMAL_EPSILON ) + ( fabs( normals[j][1] ) < NORMAL_EPSILON ) + ( fabs( normals[j][2] ) < NORMAL_EPSILON );
                        if ( axial > bestaxial )
                        {
                                bestside = j;
                                bestaxial = axial;
                        }
                }
                sorted[i] = bestside;
                isused[bestside] = true;
        }
        for ( i = -1; i < numsides; i++ )
        {
                *( i >= 0 ? &sides[sorted[i]]->next : &h->faces ) = ( i + 1 < numsides ? sides[sorted[i + 1]] : NULL );
        }
        free( sides );
        free( normals );
        free( isused );
        free( sorted );
}
void            MakeHullFaces( const brush_t* const b, brushhull_t *h )
{
        bface_t*        f;
        bface_t*        f2;
        // this will decrease AllocBlock amount
        SortSides( h );

restart:
        h->bounds.reset();

        // for each face in this brushes hull
        for ( f = h->faces; f; f = f->next )
        {
                Winding* w = new Winding( f->plane->normal, f->plane->dist );
                for ( f2 = h->faces; f2; f2 = f2->next )
                {
                        if ( f == f2 )
                        {
                                continue;
                        }
                        const plane_t* p = &g_mapplanes[f2->planenum ^ 1];
                        if ( !w->Chop( p->normal, p->dist
                                       , NORMAL_EPSILON  // fix "invalid brush" in ExpandBrush
                        ) )   // Nothing left to chop (getArea will return 0 for us in this case for below)
                        {
                                break;
                        }
                }
                w->RemoveColinearPoints( ON_EPSILON );
                if ( w->getArea() < 0.1 )
                {
                        delete w;
                        if ( h->faces == f )
                        {
                                h->faces = f->next;
                        }
                        else
                        {
                                for ( f2 = h->faces; f2->next != f; f2 = f2->next )
                                {
                                        ;
                                }
                                f2->next = f->next;
                        }
                        goto restart;
                }
                else
                {
                        f->w = w;
                        f->contents = CONTENTS_EMPTY;
                        unsigned int    i;
                        for ( i = 0; i < w->m_NumPoints; i++ )
                        {
                                h->bounds.add( w->m_Points[i] );
                        }
                }
        }

        unsigned int    i;
        for ( i = 0; i < 3; i++ )
        {
                if ( h->bounds.m_Mins[i] < -BOGUS_RANGE / 2 || h->bounds.m_Maxs[i] > BOGUS_RANGE / 2 )
                {
                        Fatal( assume_BRUSH_OUTSIDE_WORLD, "Entity %i, Brush %i: outside world(+/-%d): (%.0f,%.0f,%.0f)-(%.0f,%.0f,%.0f)",
                               b->originalentitynum, b->originalbrushnum,
                               BOGUS_RANGE / 2,
                               h->bounds.m_Mins[0], h->bounds.m_Mins[1], h->bounds.m_Mins[2],
                               h->bounds.m_Maxs[0], h->bounds.m_Maxs[1], h->bounds.m_Maxs[2] );
                }
        }
}

// =====================================================================================
//  MakeBrushPlanes
// =====================================================================================
bool            MakeBrushPlanes( brush_t* b, int brushnum )
{
        int             i;
        int             j;
        int             planenum;
        side_t*         s;
        bface_t*        f;
        vec3_t          origin;

        //
        // if the origin key is set (by an origin brush), offset all of the values
        //
        GetVectorDForKey( &g_bspdata->entities[b->entitynum], "origin", origin );

        //
        // convert to mapplanes
        //
        // for each side in this brush
        for ( i = 0; i < b->numsides; i++ )
        {
                s = &g_brushsides[b->firstside + i];
                for ( j = 0; j < 3; j++ )
                {
                        VectorSubtract( s->planepts[j], origin, s->planepts[j] );
                }
                planenum = PlaneFromPoints( s->planepts[0], s->planepts[1], s->planepts[2] );
                if ( planenum == -1 )
                {
                        Fatal( assume_PLANE_WITH_NO_NORMAL, "Entity %i, Brush %i, Side %i: plane with no normal",
                               b->originalentitynum, b->originalbrushnum
                               , i );
                }

                //
                // see if the plane has been used already
                //
                for ( f = b->hulls[0].faces; f; f = f->next )
                {
                        if ( f->planenum == planenum || f->planenum == ( planenum ^ 1 ) )
                        {
                                Fatal( assume_BRUSH_WITH_COPLANAR_FACES, "Entity %i, Brush %i, Side %i: has a coplanar plane at (%.0f, %.0f, %.0f), texture %s",
                                       b->originalentitynum, b->originalbrushnum
                                       , i, s->planepts[0][0] + origin[0], s->planepts[0][1] + origin[1],
                                       s->planepts[0][2] + origin[2], s->td.name );
                        }
                }

                f = (bface_t*)Alloc( sizeof( *f ) );                             // TODO: This leaks

                f->planenum = planenum;
                f->plane = &g_mapplanes[planenum];
                f->next = b->hulls[0].faces;
                b->hulls[0].faces = f;
                f->texinfo = g_onlyents ? 0 : TexinfoForBrushTexture( f->plane, &s->td, origin
                );
                s->texinfo = f->texinfo;
                s->planenum = f->planenum;
                f->bevel = b->bevel || s->bevel;
                f->brushnum = brushnum;
                f->brushside = i;
        }

        return true;
}

// =====================================================================================
//  TextureContents
// =====================================================================================
static contents_t TextureContents( const char* const name )
{
        contents_t cts = GetTextureContents( name );
        return cts;
}

// =====================================================================================
//  ContentsToString
// =====================================================================================
const char*     ContentsToString( const contents_t type )
{
        switch ( type )
        {
        case CONTENTS_EMPTY:
                return "EMPTY";
        case CONTENTS_SOLID:
                return "SOLID";
        case CONTENTS_WATER:
                return "WATER";
        case CONTENTS_SLIME:
                return "SLIME";
        case CONTENTS_LAVA:
                return "LAVA";
        case CONTENTS_SKY:
                return "SKY";
        case CONTENTS_ORIGIN:
                return "ORIGIN";
        case CONTENTS_BOUNDINGBOX:
                return "BOUNDINGBOX";
        case CONTENTS_CURRENT_0:
                return "CURRENT_0";
        case CONTENTS_CURRENT_90:
                return "CURRENT_90";
        case CONTENTS_CURRENT_180:
                return "CURRENT_180";
        case CONTENTS_CURRENT_270:
                return "CURRENT_270";
        case CONTENTS_CURRENT_UP:
                return "CURRENT_UP";
        case CONTENTS_CURRENT_DOWN:
                return "CURRENT_DOWN";
        case CONTENTS_TRANSLUCENT:
                return "TRANSLUCENT";
        case CONTENTS_HINT:
                return "HINT";

        case CONTENTS_NULL:
                return "NULL";


        case CONTENTS_TOEMPTY:
                return "EMPTY";

        default:
                return "UNKNOWN";
        }
}

// =====================================================================================
//  CheckBrushContents
//      Perfoms abitrary checking on brush surfaces and states to try and catch errors
// =====================================================================================
contents_t      CheckBrushContents( const brush_t* const b )
{
        contents_t      best_contents;
        contents_t      contents;
        side_t*         s;
        int             i;
        int				best_i;
        bool			assigned = false;

        s = &g_brushsides[b->firstside];

        // cycle though the sides of the brush and attempt to get our best side contents for
        //  determining overall brush contents
        if ( b->numsides == 0 )
        {
                Error( "Entity %i, Brush %i: Brush with no sides.\n",
                       b->originalentitynum, b->originalbrushnum
                );
        }
        best_i = 0;
        best_contents = TextureContents( s->td.name );
        // Difference between SKIP, ContentEmpty:
        // SKIP doesn't split space in bsp process, ContentEmpty splits space normally.
        if ( !( strncasecmp( s->td.name, "content", 7 ) && strncasecmp( s->td.name, "skip", 4 ) ) )
                assigned = true;
        s++;
        for ( i = 1; i < b->numsides; i++, s++ )
        {
                contents_t contents_consider = TextureContents( s->td.name );
                if ( assigned )
                        continue;
                if ( !( strncasecmp( s->td.name, "content", 7 ) && strncasecmp( s->td.name, "skip", 4 ) ) )
                {
                        best_i = i;
                        best_contents = contents_consider;
                        assigned = true;
                }
                if ( contents_consider < best_contents )
                {
                        best_i = i;
                        // if our current surface contents is better (smaller) than our best, make it our best.
                        best_contents = contents_consider;
                }
        }
        contents = best_contents;

        // attempt to pick up on mixed_face_contents errors
        s = &g_brushsides[b->firstside];
        for ( i = 0; i < b->numsides; i++, s++ )
        {
                contents_t contents2 = TextureContents( s->td.name );
                if ( assigned
                     && strncasecmp( s->td.name, "content", 7 )
                     && strncasecmp( s->td.name, "skip", 4 )
                     && contents2 != CONTENTS_ORIGIN
                     && contents2 != CONTENTS_HINT
                     && contents2 != CONTENTS_BOUNDINGBOX
                     )
                {
                        continue; // overwrite content for this texture
                }

                // AJM: sky and null types are not to cause mixed face contents
                if ( contents2 == CONTENTS_SKY )
                        continue;

                if ( contents2 == CONTENTS_NULL )
                        continue;

                if ( contents2 != best_contents )
                {
                        Fatal( assume_MIXED_FACE_CONTENTS, "Entity %i, Brush %i: mixed face contents\n    Texture %s and %s",
                               b->originalentitynum, b->originalbrushnum,
                               g_brushsides[b->firstside + best_i].td.name,
                               s->td.name );
                }
        }
        if ( contents == CONTENTS_NULL )
                contents = CONTENTS_SOLID;

        // check to make sure we dont have an origin brush as part of worldspawn
        if ( ( b->entitynum == 0 ) || ( strcmp( "func_group", ValueForKey( &g_bspdata->entities[b->entitynum], "classname" ) ) == 0 ) )
        {
                if ( contents == CONTENTS_ORIGIN
                     && b->entitynum == 0
                     || contents == CONTENTS_BOUNDINGBOX
                     )
                {
                        Fatal( assume_BRUSH_NOT_ALLOWED_IN_WORLD, "Entity %i, Brush %i: %s brushes not allowed in world\n(did you forget to tie this origin brush to a rotating entity?)",
                               b->originalentitynum, b->originalbrushnum,
                               ContentsToString( contents ) );
                }
        }
        else
        {
                // otherwise its not worldspawn, therefore its an entity. check to make sure this brush is allowed
                //  to be an entity.
                switch ( contents )
                {
                case CONTENTS_SOLID:
                case CONTENTS_WATER:
                case CONTENTS_SLIME:
                case CONTENTS_LAVA:
                case CONTENTS_ORIGIN:
                case CONTENTS_BOUNDINGBOX:
                case CONTENTS_HINT:
                case CONTENTS_TOEMPTY:
                        break;
                default:
                        Fatal( assume_BRUSH_NOT_ALLOWED_IN_ENTITY, "Entity %i, Brush %i: %s brushes not allowed in entity",
                               b->originalentitynum, b->originalbrushnum,
                               ContentsToString( contents ) );
                        break;
                }
        }

        return contents;
}


// =====================================================================================
//  CreateBrush
//      makes a brush!
// =====================================================================================
void CreateBrush( const int brushnum ) //--vluzacn
{
        brush_t*        b;
        int             contents;
        int             h;

        b = &g_mapbrushes[brushnum];

        contents = b->contents;

        if ( contents == CONTENTS_ORIGIN )
                return;
        if ( contents == CONTENTS_BOUNDINGBOX )
                return;

        //  HULL 0
        MakeBrushPlanes( b, brushnum );
        MakeHullFaces( b, &b->hulls[0] );

        if ( contents == CONTENTS_HINT )
                return;
        if ( contents == CONTENTS_TOEMPTY )
                return;
}
hullbrush_t *CreateHullBrush( const brush_t *b )
{
        const int MAXSIZE = 256;

        hullbrush_t *hb;
        int numplanes;
        plane_t planes[MAXSIZE];
        Winding *w[MAXSIZE];
        int numedges;
        hullbrushedge_t edges[MAXSIZE];
        int numvertexes;
        hullbrushvertex_t vertexes[MAXSIZE];
        int i;
        int j;
        int k;
        int e;
        int e2;
        vec3_t origin;
        bool failed = false;

        // planes

        numplanes = 0;
        GetVectorDForKey( &g_bspdata->entities[b->entitynum], "origin", origin );

        for ( i = 0; i < b->numsides; i++ )
        {
                side_t *s;
                vec3_t p[3];
                vec3_t v1;
                vec3_t v2;
                vec3_t normal;
                planetypes axial;

                s = &g_brushsides[b->firstside + i];
                for ( j = 0; j < 3; j++ )
                {
                        VectorSubtract( s->planepts[j], origin, p[j] );
                        for ( k = 0; k < 3; k++ )
                        {
                                if ( fabs( p[j][k] - floor( p[j][k] + 0.5 ) ) <= ON_EPSILON && p[j][k] != floor( p[j][k] + 0.5 ) )
                                {
                                        Warning( "Entity %i, Brush %i: vertex (%4.8f %4.8f %4.8f) of an info_hullshape entity is slightly off-grid.",
                                                 b->originalentitynum, b->originalbrushnum,
                                                 p[j][0], p[j][1], p[j][2] );
                                }
                        }
                }
                VectorSubtract( p[0], p[1], v1 );
                VectorSubtract( p[2], p[1], v2 );
                CrossProduct( v1, v2, normal );
                if ( !VectorNormalize( normal ) )
                {
                        failed = true;
                        continue;
                }
                for ( k = 0; k < 3; k++ )
                {
                        if ( fabs( normal[k] ) < NORMAL_EPSILON )
                        {
                                normal[k] = 0.0;
                                VectorNormalize( normal );
                        }
                }
                axial = PlaneTypeForNormal( normal );
                if ( axial <= last_axial )
                {
                        int sign = normal[axial] > 0 ? 1 : -1;
                        VectorClear( normal );
                        normal[axial] = sign;
                }

                if ( numplanes >= MAXSIZE )
                {
                        failed = true;
                        continue;
                }
                VectorCopy( normal, planes[numplanes].normal );
                planes[numplanes].dist = DotProduct( p[1], normal );
                numplanes++;
        }

        // windings

        for ( i = 0; i < numplanes; i++ )
        {
                w[i] = new Winding( planes[i].normal, planes[i].dist );
                for ( j = 0; j < numplanes; j++ )
                {
                        if ( j == i )
                        {
                                continue;
                        }
                        vec3_t normal;
                        vec_t dist;
                        VectorSubtract( vec3_origin, planes[j].normal, normal );
                        dist = -planes[j].dist;
                        if ( !w[i]->Chop( normal, dist ) )
                        {
                                failed = true;
                                break;
                        }
                }
        }

        // edges
        numedges = 0;
        for ( i = 0; i < numplanes; i++ )
        {
                for ( e = 0; e < w[i]->m_NumPoints; e++ )
                {
                        hullbrushedge_t *edge;
                        int found;
                        if ( numedges >= MAXSIZE )
                        {
                                failed = true;
                                continue;
                        }
                        edge = &edges[numedges];
                        VectorCopy( w[i]->m_Points[( e + 1 ) % w[i]->m_NumPoints], edge->vertexes[0] );
                        VectorCopy( w[i]->m_Points[e], edge->vertexes[1] );
                        VectorCopy( edge->vertexes[0], edge->point );
                        VectorSubtract( edge->vertexes[1], edge->vertexes[0], edge->delta );
                        if ( VectorLength( edge->delta ) < 1 - ON_EPSILON )
                        {
                                failed = true;
                                continue;
                        }
                        VectorCopy( planes[i].normal, edge->normals[0] );
                        found = 0;
                        for ( k = 0; k < numplanes; k++ )
                        {
                                for ( e2 = 0; e2 < w[k]->m_NumPoints; e2++ )
                                {
                                        if ( VectorCompareD( w[k]->m_Points[( e2 + 1 ) % w[k]->m_NumPoints], edge->vertexes[1] ) &&
                                             VectorCompareD( w[k]->m_Points[e2], edge->vertexes[0] ) )
                                        {
                                                found++;
                                                VectorCopy( planes[k].normal, edge->normals[1] );
                                                j = k;
                                        }
                                }
                        }
                        if ( found != 1 )
                        {
                                failed = true;
                                continue;
                        }
                        if ( fabs( DotProduct( edge->vertexes[0], edge->normals[0] ) - planes[i].dist ) > NORMAL_EPSILON
                             || fabs( DotProduct( edge->vertexes[1], edge->normals[0] ) - planes[i].dist ) > NORMAL_EPSILON
                             || fabs( DotProduct( edge->vertexes[0], edge->normals[1] ) - planes[j].dist ) > NORMAL_EPSILON
                             || fabs( DotProduct( edge->vertexes[1], edge->normals[1] ) - planes[j].dist ) > NORMAL_EPSILON )
                        {
                                failed = true;
                                continue;
                        }
                        if ( j > i )
                        {
                                numedges++;
                        }
                }
        }

        // vertexes
        numvertexes = 0;
        for ( i = 0; i < numplanes; i++ )
        {
                for ( e = 0; e < w[i]->m_NumPoints; e++ )
                {
                        vec3_t v;
                        VectorCopy( w[i]->m_Points[e], v );
                        for ( j = 0; j < numvertexes; j++ )
                        {
                                if ( VectorCompareD( vertexes[j].point, v ) )
                                {
                                        break;
                                }
                        }
                        if ( j < numvertexes )
                        {
                                continue;
                        }
                        if ( numvertexes > MAXSIZE )
                        {
                                failed = true;
                                continue;
                        }

                        VectorCopy( v, vertexes[numvertexes].point );
                        numvertexes++;

                        for ( k = 0; k < numplanes; k++ )
                        {
                                if ( fabs( DotProduct( v, planes[k].normal ) - planes[k].dist ) < ON_EPSILON )
                                {
                                        if ( fabs( DotProduct( v, planes[k].normal ) - planes[k].dist ) > NORMAL_EPSILON )
                                        {
                                                failed = true;
                                        }
                                }
                        }
                }
        }

        // copy to hull brush

        if ( !failed )
        {
                hb = (hullbrush_t *)malloc( sizeof( hullbrush_t ) );
                hlassume( hb != NULL, assume_NoMemory );

                hb->numfaces = numplanes;
                hb->faces = (hullbrushface_t *)malloc( hb->numfaces * sizeof( hullbrushface_t ) );
                hlassume( hb->faces != NULL, assume_NoMemory );
                for ( i = 0; i < numplanes; i++ )
                {
                        hullbrushface_t *f = &hb->faces[i];
                        VectorCopy( planes[i].normal, f->normal );
                        VectorCopy( w[i]->m_Points[0], f->point );
                        f->numvertexes = w[i]->m_NumPoints;
                        f->vertexes = (vec3_t *)malloc( f->numvertexes * sizeof( vec3_t ) );
                        hlassume( f->vertexes != NULL, assume_NoMemory );
                        for ( k = 0; k < w[i]->m_NumPoints; k++ )
                        {
                                VectorCopy( w[i]->m_Points[k], f->vertexes[k] );
                        }
                }

                hb->numedges = numedges;
                hb->edges = (hullbrushedge_t *)malloc( hb->numedges * sizeof( hullbrushedge_t ) );
                hlassume( hb->edges != NULL, assume_NoMemory );
                memcpy( hb->edges, edges, hb->numedges * sizeof( hullbrushedge_t ) );

                hb->numvertexes = numvertexes;
                hb->vertexes = (hullbrushvertex_t *)malloc( hb->numvertexes * sizeof( hullbrushvertex_t ) );
                hlassume( hb->vertexes != NULL, assume_NoMemory );
                memcpy( hb->vertexes, vertexes, hb->numvertexes * sizeof( hullbrushvertex_t ) );

                Developer( DEVELOPER_LEVEL_MESSAGE, "info_hullshape @ (%.0f,%.0f,%.0f): %d faces, %d edges, %d vertexes.\n", origin[0], origin[1], origin[2], hb->numfaces, hb->numedges, hb->numvertexes );
        }
        else
        {
                hb = NULL;
                Error( "Entity %i, Brush %i: invalid brush. This brush cannot be used for info_hullshape.",
                       b->originalentitynum, b->originalbrushnum
                );
        }

        for ( i = 0; i < numplanes; i++ )
        {
                delete w[i];
        }

        return hb;
}

hullbrush_t *CopyHullBrush( const hullbrush_t *hb )
{
        hullbrush_t *hb2;
        hb2 = (hullbrush_t *)malloc( sizeof( hullbrush_t ) );
        hlassume( hb2 != NULL, assume_NoMemory );
        memcpy( hb2, hb, sizeof( hullbrush_t ) );
        hb2->faces = (hullbrushface_t *)malloc( hb->numfaces * sizeof( hullbrushface_t ) );
        hlassume( hb2->faces != NULL, assume_NoMemory );
        memcpy( hb2->faces, hb->faces, hb->numfaces * sizeof( hullbrushface_t ) );
        hb2->edges = (hullbrushedge_t *)malloc( hb->numedges * sizeof( hullbrushedge_t ) );
        hlassume( hb2->edges != NULL, assume_NoMemory );
        memcpy( hb2->edges, hb->edges, hb->numedges * sizeof( hullbrushedge_t ) );
        hb2->vertexes = (hullbrushvertex_t *)malloc( hb->numvertexes * sizeof( hullbrushvertex_t ) );
        hlassume( hb2->vertexes != NULL, assume_NoMemory );
        memcpy( hb2->vertexes, hb->vertexes, hb->numvertexes * sizeof( hullbrushvertex_t ) );
        for ( int i = 0; i < hb->numfaces; i++ )
        {
                hullbrushface_t *f2 = &hb2->faces[i];
                const hullbrushface_t *f = &hb->faces[i];
                f2->vertexes = (vec3_t *)malloc( f->numvertexes * sizeof( vec3_t ) );
                hlassume( f2->vertexes != NULL, assume_NoMemory );
                memcpy( f2->vertexes, f->vertexes, f->numvertexes * sizeof( vec3_t ) );
        }
        return hb2;
}

void DeleteHullBrush( hullbrush_t *hb )
{
        for ( hullbrushface_t *hbf = hb->faces; hbf < hb->faces + hb->numfaces; hbf++ )
        {
                if ( hbf->vertexes )
                {
                        free( hbf->vertexes );
                }
        }
        free( hb->faces );
        free( hb->edges );
        free( hb->vertexes );
        free( hb );
}

void InitDefaultHulls()
{
        for ( int h = 0; h < NUM_HULLS; h++ )
        {
                hullshape_t *hs = &g_defaulthulls[h];
                hs->id = _strdup( "" );
                hs->disabled = true;
                hs->numbrushes = 0;
                hs->brushes = (hullbrush_t **)malloc( 0 * sizeof( hullbrush_t * ) );
                hlassume( hs->brushes != NULL, assume_NoMemory );
        }
}

void CreateHullShape( int entitynum, bool disabled, const char *id, int defaulthulls )
{
        entity_t *entity;
        hullshape_t *hs;

        entity = &g_bspdata->entities[entitynum];
        if ( !*ValueForKey( entity, "origin" ) )
        {
                Warning( "info_hullshape with no ORIGIN brush." );
        }
        if ( g_numhullshapes >= MAX_HULLSHAPES )
        {
                Error( "Too many info_hullshape entities. Can not exceed %d.", MAX_HULLSHAPES );
        }
        hs = &g_hullshapes[g_numhullshapes];
        g_numhullshapes++;

        hs->id = _strdup( id );
        hs->disabled = disabled;
        hs->numbrushes = 0;
        hs->brushes = (hullbrush_t **)malloc( entity->numbrushes * sizeof( hullbrush_t * ) );
        for ( int i = 0; i < entity->numbrushes; i++ )
        {
                brush_t *b = &g_mapbrushes[entity->firstbrush + i];
                if ( b->contents == CONTENTS_ORIGIN )
                {
                        continue;
                }
                hs->brushes[hs->numbrushes] = CreateHullBrush( b );
                hs->numbrushes++;
        }
        if ( hs->numbrushes >= 2 )
        {
                brush_t *b = &g_mapbrushes[entity->firstbrush];
                Error( "Entity %i, Brush %i: Too many brushes in info_hullshape.",
                       b->originalentitynum, b->originalbrushnum
                );
        }

        for ( int h = 0; h < NUM_HULLS; h++ )
        {
                if ( defaulthulls & ( 1 << h ) )
                {
                        hullshape_t *target = &g_defaulthulls[h];
                        for ( int i = 0; i < target->numbrushes; i++ )
                        {
                                DeleteHullBrush( target->brushes[i] );
                        }
                        free( target->brushes );
                        free( target->id );
                        target->id = _strdup( hs->id );
                        target->disabled = hs->disabled;
                        target->numbrushes = hs->numbrushes;
                        target->brushes = (hullbrush_t **)malloc( hs->numbrushes * sizeof( hullbrush_t * ) );
                        hlassume( target->brushes != NULL, assume_NoMemory );
                        for ( int i = 0; i < hs->numbrushes; i++ )
                        {
                                target->brushes[i] = CopyHullBrush( hs->brushes[i] );
                        }
                }
        }
}
