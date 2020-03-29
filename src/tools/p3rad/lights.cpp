/**
 * PANDA3D BSP TOOLS
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file lights.cpp
 * @author Brian Lach
 * @date November 20, 2018
 *
 */

//#include "clhelper.h"
#include "lights.h"
#include "qrad.h"
#include "bsptools.h"

directlight_t *Lights::activelights = nullptr;
directlight_t* Lights::directlights[MAX_MAP_LEAFS];
int Lights::numdlights = 0;
float Lights::sun_angular_extent = 0.0;
directlight_t *Lights::skylight = nullptr;
directlight_t *Lights::ambientlight = nullptr;

#define VIS_SIZE ( MAX_MAP_LEAFS + 7 ) / 8

int GetVisCache( int lastoffset, int cluster, byte *pvs )
{
        if ( !g_bspdata->visdatasize )
        {
                memset( pvs, 255, VIS_SIZE );
                lastoffset = -1;
        }
        else
        {
                if ( cluster < 0 )
                {
                        // erorr point embedded in wall
                        memset( pvs, 255, VIS_SIZE );
                        lastoffset = -1;
                }
                else
                {
                        int thisoffset = g_bspdata->dleafs[cluster].visofs;
                        if ( thisoffset != lastoffset )
                        {
                                if ( thisoffset == -1 )
                                {
                                        memset( pvs, 0, VIS_SIZE );
                                        Error( "visofs == -1" );
                                }

                                DecompressVis( g_bspdata, &g_bspdata->dvisdata[thisoffset], pvs, VIS_SIZE );
                        }
                        lastoffset = thisoffset;
                }
        }

        return lastoffset;
}

void SetDLightVis( directlight_t *dl, int leafnum )
{
        if ( dl->pvs == nullptr )
        {
                dl->pvs = (byte *)calloc( 1, VIS_SIZE );
        }

        GetVisCache( -1, leafnum, dl->pvs );
}

void MergeDLightVis( directlight_t *dl, int leafnum )
{
        if ( dl->pvs == nullptr )
        {
                SetDLightVis( dl, leafnum );
        }
        else
        {
                byte pvs[VIS_SIZE];
                GetVisCache( -1, leafnum, pvs );

                // merge both vis graphs
                for ( int i = 0; i < GetNumWorldLeafs( g_bspdata ) + 1; i++ )
                {
                        dl->pvs[i] |= pvs[i];
                }
        }
}

void AddDLightToActiveList( directlight_t *dl )
{
        dl->next = Lights::activelights;
        Lights::activelights = dl;
}

directlight_t *AllocDLight( LVector3 &origin, bool add_to_list )
{
        directlight_t *dl = (directlight_t *)calloc( 1, sizeof( directlight_t ) );
        dl->index = Lights::numdlights++;

        VectorCopy( origin, dl->origin );
        dl->leaf = PointInLeaf( dl->origin ) - g_bspdata->dleafs;
        SetDLightVis( dl, dl->leaf );

        dl->facenum = -1;

        if ( add_to_list )
        {
                AddDLightToActiveList( dl );
        }
        else
        {
                dl->next = nullptr;
        }

        return dl;
}

bool LightForString( const char *light, LVector3 &intensity )
{
        double r, g, b, scalar;
        int arg_cnt;

        intensity.set( 0.0, 0.0, 0.0 );

        // scanf into doubles, then assign, so it is vec_t size independent
        r = g = b = scalar = 0;
        arg_cnt = sscanf( light, "%lf %lf %lf %lf", &r, &g, &b, &scalar );

        // make sure light is legal
        if ( r < 0.0 || g < 0.0 || b < 0.0 || scalar < 0.0 )
        {
                return false;
        }

        intensity[0] = std::pow( r / 255.0, 2.2 ) * 255.0;
        switch ( arg_cnt )
        {
        case 1:
                // rgb vals equal
                intensity[1] = intensity[2] = intensity[0];
                break;
        case 3:
        case 4:
                intensity[1] = std::pow( g / 255.0, 2.2 ) * 255;
                intensity[2] = std::pow( b / 255.0, 2.2 ) * 255;

                // did we also get a scalar?
                if ( arg_cnt == 4 )
                {
                        // scale the normalized 0-255 rgb values by the intensity scalar
                        VectorScale( intensity, scalar / 255, intensity );
                }
                break;
        }

        // scale up source lights by scaling factor
        VectorScale( intensity, g_lightscale, intensity );
        return true;
}

bool LightForKey( entity_t *ent, char *key, LVector3 &intensity )
{
        const char *light;
        light = ValueForKey( ent, key );

        return LightForString( light, intensity );
}

void SetupLightNormalFromProps( const LVector3 &angles, float angle, float pitch, LVector3 &normal )
{
        if ( angle == ANGLE_UP )
        {
                normal[0] = normal[1] = 0;
                normal[2] = 1;
        }
        else if ( angle == ANGLE_DOWN )
        {
                normal[0] = normal[1] = 0;
                normal[2] = -1;
        }
        else
        {
                // if we don't have a specific "angle" use the "angles" yaw
                if ( !angle )
                {
                        angle = angles[1];
                }

                normal[2] = 0;
                normal[0] = (float)std::cos( angle / 180 * Q_PI );
                normal[1] = (float)std::sin( angle / 180 * Q_PI );
        }

        if ( !pitch )
        {
                pitch = angles[0];
        }

        normal[2] = (float)std::sin( pitch / 180 * Q_PI );
        normal[0] *= (float)std::cos( pitch / 180 * Q_PI );
        normal[1] *= (float)std::cos( pitch / 180 * Q_PI );
}

void ParseLightGeneric( entity_t *e, directlight_t *dl )
{
        entity_t *e2;
        const char *target;
        vec3_t dest;

        dl->style = (int)FloatForKey( e, "style" );

        // get intensity
        LVector3 intensity;
        LightForKey( e, "_light", intensity );
        VectorCopy( intensity, dl->intensity );

        // check angle, targets
        target = ValueForKey( e, "target" );
        if ( target[0] )
        {
                // point towards target
                e2 = FindTargetEntity( g_bspdata, target );
                if ( !e2 )
                {
                        Warning( "light at (%i %i %i) has missing target",
                                (int)dl->origin[0], (int)dl->origin[1], (int)dl->origin[2] );
                }
                else
                {
                        GetVectorDForKey( e2, "origin", dest );
                        VectorSubtract( dest, dl->origin, dl->normal );
                        dl->normal.normalize();
                }
        }
        else
        {
                vec3_t angles;
                GetVectorDForKey( e, "angles", angles );
                float pitch = FloatForKey( e, "pitch" );
                float angle = FloatForKey( e, "angle" );
                SetupLightNormalFromProps( GetLVector3( angles ), angle, pitch, dl->normal );
        }
}

void SetLightFalloffParams( entity_t *e, directlight_t *dl )
{
        lightfalloffparams_t params = GetLightFalloffParams( e, dl->intensity );
        dl->constant_atten = params.constant_atten;
        dl->linear_atten = params.linear_atten;
        dl->quadratic_atten = params.quadratic_atten;
        dl->radius = params.radius;
        dl->start_fade_distance = params.start_fade_distance;
        dl->end_fade_distance = params.end_fade_distance;
        dl->cap_distance = params.cap_distance;
}

void ParseLightSpot( entity_t *e, directlight_t *dl )
{
        vec3_t dest;
        GetVectorDForKey( e, "origin", dest );
        dl = AllocDLight( GetLVector3( dest ), true );

        ParseLightGeneric( e, dl );

        dl->type = emit_spotlight;

        dl->stopdot = FloatForKey( e, "_inner_cone" );
        if ( !dl->stopdot )
                dl->stopdot = 10;

        dl->stopdot2 = FloatForKey( e, "_cone" );
        if ( !dl->stopdot2 || dl->stopdot2 < dl->stopdot )
                dl->stopdot2 = dl->stopdot;

        // this is a point light is stop dots are 180
        if ( dl->stopdot == 180 && dl->stopdot2 == 180 )
        {
                dl->stopdot = dl->stopdot2 = 0;
                dl->type = emit_point;
                dl->exponent = 0;
        }
        else
        {
                dl->stopdot2 = (float)std::cos( dl->stopdot2 / 180 * Q_PI );
                dl->stopdot = (float)std::cos( dl->stopdot / 180 * Q_PI );
                dl->exponent = FloatForKey( e, "_exponent" );
        }

        SetLightFalloffParams( e, dl );
}

static char *ValueForKeyWithDefault( entity_t *ent, char *key, char *default_value = NULL )
{
        epair_t	*ep;

        for ( ep = ent->epairs; ep; ep = ep->next )
        {
                if ( !strcmp( ep->key, key ) )
                        return ep->value;
        }
                
        return default_value;
}

/**
 * Since light_environments span over all skybox brushes (not just a single point like other lights),
 * we need to figure out a special PVS that determines which leafs can see a sky brush.
 */
void BuildVisForLightEnvironment()
{
        dleaf_t *dleafs = g_bspdata->dleafs;
        int numleafs = g_bspdata->numleafs;

        // create the vis
        for ( int leafnum = 0; leafnum < numleafs; leafnum++ )
        {
                // clear any existing sky flags
                dleafs[leafnum].flags &= ~( LEAF_FLAGS_SKY | LEAF_FLAGS_SKY2D );

                // figure out if this leaf contains sky brushes

                size_t firstface = dleafs[leafnum].firstmarksurface;
                for ( size_t leafface = 0; leafface < dleafs[leafnum].nummarksurfaces; leafface++ )
                {
                        size_t iface = g_bspdata->dmarksurfaces[firstface + leafface];

                        texinfo_t *tex = &g_bspdata->texinfo[g_bspdata->dfaces[iface].texinfo];
                        if ( tex->flags & TEX_SKY )
                        {
                                if ( tex->flags & TEX_SKY2D )
                                {
                                        dleafs[leafnum].flags |= LEAF_FLAGS_SKY2D;
                                }
                                else
                                {
                                        dleafs[leafnum].flags |= LEAF_FLAGS_SKY;
                                }
                                // yes, this leaf contains a sky brush (me), add it to the PVS for the
                                // lights
                                MergeDLightVis( Lights::skylight, leafnum );
                                MergeDLightVis( Lights::ambientlight, leafnum );
                                break;
                        }
                }
        }

        // second pass to set flags on leaves that don't contain sky, but touch leaves that do
        byte pvs[( MAX_MAP_LEAFS + 7 ) / 8];
        int leaf_bytes = ( numleafs >> 3 ) + 1;
        byte *leafbits = (byte *)calloc( leaf_bytes, sizeof( byte ) );
        byte *leaf2dbits = (byte *)calloc( leaf_bytes, sizeof( byte ) );
        memset( leafbits, 0, leaf_bytes );
        memset( leaf2dbits, 0, leaf_bytes );

        for ( int ileaf = 0; ileaf < numleafs; ileaf++ )
        {
                // if this leaf has light (3d skybox) in it, then don't bother
                if ( dleafs[ileaf].flags & LEAF_FLAGS_SKY )
                        continue;

                // dont bother with solid leafs
                if ( dleafs[ileaf].contents & CONTENTS_SOLID )
                        continue;

                GetVisCache( -1, ileaf, pvs );

                // now check out all other leafs
                int nbyte = ileaf >> 3;
                int nbit = 1 << ( ileaf & 0x7 );
                for ( int ileaf2 = 0; ileaf2 < numleafs; ileaf2++ )
                {
                        if ( ileaf2 == ileaf )
                                continue;

                        if ( !( dleafs[ileaf2].flags & ( LEAF_FLAGS_SKY | LEAF_FLAGS_SKY2D ) ) )
                        {
                                // leaf doesn't contain sky
                                continue;
                        } 

                        // Can this leaf see into the leaf with the sky in it?
                        if ( !PVSCheck( pvs, ileaf2 ) )
                        {
                                // nope, we can't see the sky leaf
                                continue;
                        }
                                

                        if ( dleafs[ileaf2].flags & LEAF_FLAGS_SKY2D )
                        {
                                // we can see a leaf with a 2d sky in it
                                leaf2dbits[nbyte] |= nbit;
                        }
                                
                        if ( dleafs[ileaf2].flags & LEAF_FLAGS_SKY )
                        {
                                leafbits[nbyte] |= nbit;
                                // as soon as we know this leaf needs to do the 3d skybox, we're done
                                break;
                        }
                }
        }

        // must set the bits in a separate pass so as to not flood-fill LEAF_FLAGS_SKY everywhere
        // leafbits is a bit array of all leaves that need to be marked as seeing sky
        for ( int ileaf = 0; ileaf < numleafs; ileaf++ )
        {
                if ( dleafs[ileaf].flags & LEAF_FLAGS_SKY )
                        continue;

                if ( dleafs[ileaf].contents & CONTENTS_SOLID )
                        continue;

                if ( leaf2dbits[ileaf >> 3] & ( 1 << ( ileaf & 0x7 ) ) )
                {
                        // mark this leaf as seeing the sky
                        dleafs[ileaf].flags |= LEAF_FLAGS_SKY2D;
                }
                        

                // todo 3d skyboxes
        }

        // dont leak memory!
        free( leafbits );
        free( leaf2dbits );
}

void ParseLightEnvironment( entity_t *e, directlight_t *dl )
{
        vec3_t dest;
        GetVectorDForKey( e, "origin", dest );
        dl = AllocDLight( GetLVector3( dest ), false );

        ParseLightGeneric( e, dl );

        char *angle_str = ValueForKeyWithDefault( e, "SunSpreadAngle" );
        if ( angle_str )
        {
                Lights::sun_angular_extent = atof( angle_str );
                Lights::sun_angular_extent = std::sin( ( Q_PI / 180.0 ) * Lights::sun_angular_extent );
                printf( "sun extent from map=%f\n", Lights::sun_angular_extent );
        }

        if ( !Lights::skylight )
        {
                Lights::skylight = dl;
                dl->type = emit_skylight;

                // sky ambient light
                Lights::ambientlight = AllocDLight( dl->origin, false );
                Lights::ambientlight->type = emit_skyambient;

                LVector3 vambint;
                if ( !LightForKey( e, "_ambient", vambint ) )
                {
                        VectorScale( dl->intensity, 0.5, Lights::ambientlight->intensity );
                }
                else
                {
                        VectorCopy( vambint, Lights::ambientlight->intensity );
                }

                BuildVisForLightEnvironment();

                // add sky and ambient to the list
                AddDLightToActiveList( Lights::skylight );
                AddDLightToActiveList( Lights::ambientlight );
        }
}

void ParseLightPoint( entity_t *e, directlight_t *dl )
{
        vec3_t dest;
        GetVectorDForKey( e, "origin", dest );
        dl = AllocDLight( GetLVector3( dest ), true );

        ParseLightGeneric( e, dl );

        dl->type = emit_point;

        SetLightFalloffParams( e, dl );
}

#define DIRECT_SCALE (100.0 * 100.0)

// =====================================================================================
//  CreateDirectLights
//  Builds the list of light sources to be used in calculation of lighting.
// =====================================================================================
void Lights::CreateDirectLights()
{
        unsigned        i;
        patch_t*        p;
        directlight_t*  dl;
        dleaf_t*        leaf;
        int             leafnum;
        entity_t*       e;
        entity_t*       e2;
        const char*     name;
        const char*     target;
        float           angle;
        vec3_t          dest;


        numdlights = 0;
        int styleused[ALLSTYLES];
        memset( styleused, 0, ALLSTYLES * sizeof( styleused[0] ) );
        styleused[0] = 1;
        int numstyles = 1;

        //
        // surfaces
        //
        for ( i = 0; i < g_patches.size(); i++ )
        {
                p = &g_patches[i];

                if ( VectorAvg( p->baselight ) >= g_dlight_threshold )
                {
                        dl = AllocDLight( p->origin, true );
                        hlassume( dl != NULL, assume_NoMemory );
                        dl->type = emit_surface;

                        // scale intensity by number of texture instances
                        VectorCopy( p->normal, dl->normal );
                        VectorScale( p->baselight, g_lightscale * p->area * p->scale[0] * p->scale[1] / p->basearea, dl->intensity );

                        // scale to a range that results in actual lights
                        VectorScale( dl->intensity, DIRECT_SCALE, dl->intensity );
                }
        }

        //
        // entities
        //
        for ( i = 0; i < (size_t)g_bspdata->numentities; i++ )
        {
                e = &g_bspdata->entities[i];
                name = ValueForKey( e, "classname" );
                if ( strncmp( name, "light", 5 ) )
                        continue;

                // Light_dynamic is actually a real entity; not to be included here...
                if ( !strcmp( name, "light_dynamic" ) )
                        continue;

                if ( !strcmp( name, "light_spot" ) )
                {
                        ParseLightSpot( e, dl );
                }
                else if ( !strcmp( name, "light_environment" ) )
                {
                        ParseLightEnvironment( e, dl );
                }
                else if ( !strcmp( name, "light" ) )
                {
                        ParseLightPoint( e, dl );
                }
                else
                {
                        printf( "unsupported light entity: \"%s\"\n", name );
                }
        }

        printf( "%i direct lights\n", numdlights );

        // now write the data into OpenCL device memory
        //CLHelper::MakeAndWriteBuffer( true, false, sizeof( int ), true, &numdlights );
        //CLHelper::MakeAndWriteBuffer( true, false, sizeof( directlight_t * ), true, activelights );
        //CLHelper::MakeAndWriteBuffer( true, false, sizeof( float ), true, &sun_angular_extent );
        //CLHelper::MakeAndWriteBuffer( true, false, sizeof( directlight_t * ), true, skylight );
        //CLHelper::MakeAndWriteBuffer( true, false, sizeof( directlight_t * ), true, ambientlight );

}

// =====================================================================================
//  DeleteDirectLights
// =====================================================================================
void Lights::DeleteDirectLights()
{
        int             l;
        directlight_t*  dl;

        for ( l = 0; l < 1 + g_bspdata->dmodels[0].visleafs; l++ )
        {
                dl = directlights[l];
                while ( dl )
                {
                        directlights[l] = dl->next;
                        free( dl );
                        dl = directlights[l];
                }
        }

        activelights = nullptr;
        numdlights = 0;
        skylight = nullptr;
        if ( ambientlight )
                free( ambientlight );
        ambientlight = nullptr;
        sun_angular_extent = 0.0;

        // AJM: todo: strip light entities out at this point
        // vluzacn: hlvis and hlrad must not modify entity data, because the following procedures are supposed to produce the same bsp file:
        //  1> hlcsg -> hlbsp -> hlvis -> hlrad  (a normal compile)
        //  2) hlcsg -> hlbsp -> hlvis -> hlrad -> hlcsg -onlyents
        //  3) hlcsg -> hlbsp -> hlvis -> hlrad -> hlcsg -onlyents -> hlrad
}

INLINE bool Lights::HasLightEnvironment()
{
        return HasSkyLight() && HasAmbientLight();
}

INLINE bool Lights::HasSkyLight()
{
        return skylight != nullptr;
}

INLINE bool Lights::HasAmbientLight()
{
        return ambientlight != nullptr;
}