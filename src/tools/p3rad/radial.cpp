#include "radial.h"

radial_t *AllocRadial( int facenum )
{
        dface_t *face = g_bspdata->dfaces + facenum;

        radial_t *rad = new radial_t;
        rad->facenum = facenum;
        rad->w = face->lightmap_size[0] + 1;
        rad->h = face->lightmap_size[1] + 1;

        return rad;
}

void FreeRadial( radial_t *rad )
{
        if ( rad != nullptr )
        {
                delete rad;
        }
}

void WorldToLuxelSpace( lightinfo_t *l, const LVector3 &world, LVector2 &coord )
{
        LVector3 pos;
        VectorSubtract( world, l->texorg, pos );
        coord[0] = DotProduct( pos, l->worldtotex[0] ) - l->face->lightmap_mins[0];
        coord[1] = DotProduct( pos, l->worldtotex[1] ) - l->face->lightmap_mins[1];
}

void WorldToLuxelSpace( lightinfo_t *l, const FourVectors &world, FourVectors &coord )
{
        FourVectors luxel_org;
        luxel_org.DuplicateVector( l->texorg );

        FourVectors pos = world;
        pos -= luxel_org;

        coord.x = pos * GetLVector3( l->worldtotex[0] );
        coord.x = SubSIMD( coord.x, ReplicateX4( l->face->lightmap_mins[0] ) );
        coord.y = pos * GetLVector3( l->worldtotex[1] );
        coord.y = SubSIMD( coord.y, ReplicateX4( l->face->lightmap_mins[1] ) );
        coord.z = Four_Zeros;
}

void LuxelToWorldSpace( lightinfo_t *l, float s, float t, LVector3 &world )
{
        LVector3 pos;
        s += l->face->lightmap_mins[0];
        t += l->face->lightmap_mins[1];
        VectorMA( l->texorg, s, l->textoworld[0], pos );
        VectorMA( pos, t, l->textoworld[1], world );
}

void LuxelToWorldSpace( lightinfo_t *l, fltx4 s, fltx4 t, FourVectors &world )
{
        world.DuplicateVector( l->texorg );
        FourVectors st;

        s = AddSIMD( s, ReplicateX4( l->face->lightmap_mins[0] ) );
        st.DuplicateVector( l->textoworld[0] );
        st *= s;
        world += st;

        t = AddSIMD( t, ReplicateX4( l->face->lightmap_mins[1] ) );
        st.DuplicateVector( l->textoworld[1] );
        st *= t;
        world += st;
}

void AddDirectToRadial( radial_t *rad, const LVector3 &pos, const LVector2 &mins,
                        const LVector2 &maxs, bumpsample_t *light,
                        bool bumpmap, bool neighbor_bumpmap )
{
        float   s_min, s_max, t_min, t_max;
        float   area;
        float   ds, dt, r;
        int     s, t;
        int     n;

        LVector2 coord;
        WorldToLuxelSpace( &lightinfo[rad->facenum], pos, coord );

        s_min = std::max( (int)mins[0], 0 );
        t_min = std::max( (int)mins[1], 0 );
        s_max = std::min( (int)( maxs[0] + 0.9999 ) + 1, rad->w );
        t_max = std::min( (int)( maxs[1] + 0.9999 ) + 1, rad->h );

        for ( s = s_min; s < s_max; s++ )
        {
                for ( t = t_min; t < t_max; t++ )
                {
                        float s0 = std::max( mins[0] - s, -1.0f );
                        float t0 = std::max( mins[1] - t, -1.0f );
                        float s1 = std::min( maxs[0] - s, 1.0f );
                        float t1 = std::min( maxs[1] - t, 1.0f );

                        area = ( s1 - s0 ) * ( t1 - t0 );

                        if ( area > EQUAL_EPSILON )
                        {
                                ds = fabs( coord[0] - s );
                                dt = fabs( coord[1] - t );

                                r = std::max( ds, dt );

                                if ( r < 0.1 )
                                {
                                        r = area / 0.1;
                                } else
                                {
                                        r = area / r;
                                }

                                int i = s + t * rad->w;

                                if ( bumpmap )
                                {
                                        if ( neighbor_bumpmap )
                                        {
                                                for ( n = 0; n < NUM_BUMP_VECTS + 1; n++ )
                                                {
                                                        rad->light[i].light[n].AddWeighted( light->light[n], r );
                                                }
                                        }
                                        else
                                        {
                                                rad->light[i].light[0].AddWeighted( light->light[0], r );
                                                for ( n = 1; n < NUM_BUMP_VECTS + 1; n++ )
                                                {
                                                        rad->light[i].light[n].AddWeighted( light->light[0], r * OO_SQRT_3 );
                                                }
                                        }
                                } else
                                {
                                        rad->light[i].light[0].AddWeighted( light->light[0], r );
                                }

                                rad->weight[i] += r;
                        }
                }
        }
}

radial_t *BuildLuxelRadial( int facenum, int style )
{
        facelight_t *fl = &facelight[facenum];
        faceneighbor_t *fn = &faceneighbor[facenum];
        dface_t *face = g_bspdata->dfaces + facenum;
        bool needs_bumpmap = face->bumped_lightmap;

        radial_t *rad = AllocRadial( facenum );

        for ( int i = 0; i < fl->numsamples; i++ )
        {
                bumpsample_t light;

                if ( needs_bumpmap )
                {
                        for ( int n = 0; n < fl->normal_count; n++ )
                        {
                                VectorCopy( fl->light[style][i].light[n], light.light[n] );
                        }
                }
                else
                {
                        VectorCopy( fl->light[style][i].light[0], light.light[0] );
                }

                LVector3 samp_pos;
                VectorCopy( fl->sample[i].pos, samp_pos );

                LVector2 samp_mins( fl->sample[i].mins[0],
                                    fl->sample[i].mins[1] );
                LVector2 samp_maxs( fl->sample[i].maxs[0],
                                    fl->sample[i].maxs[1] );

                AddDirectToRadial( rad, samp_pos, samp_mins,
                                   samp_maxs, &light, needs_bumpmap,
                                   needs_bumpmap );
        }

        
        for ( int i = 0; i < fn->numneighbors; i++ )
        {
                fl = &facelight[fn->neighbor[i]];

                bool neighbor_bump = fl->bumped;

                int nstyle = 0;

                dface_t *neighborface = g_bspdata->dfaces + fn->neighbor[i];

                // look for style that matches
                if ( neighborface->styles[nstyle] != face->styles[nstyle] )
                {
                        for ( nstyle = 1; nstyle < MAXLIGHTMAPS; nstyle++ )
                        {
                                if ( neighborface->styles[nstyle] == face->styles[nstyle] )
                                {
                                        break;
                                }
                        }

                        // if not found, skip this neighbor
                        if ( nstyle >= MAXLIGHTMAPS )
                        {
                                continue;
                        }
                }

                lightinfo_t *l = &lightinfo[fn->neighbor[i]];

                for ( int j = 0; j < fl->numsamples; j++ )
                {
                        bumpsample_t light;

                        sample_t *samp = &fl->sample[j];
                        if ( neighbor_bump )
                        {
                                for ( int n = 0; n < NUM_BUMP_VECTS + 1; n++ )
                                {
                                        VectorCopy( fl->light[style][j][n], light.light[n] );
                                }
                        }
                        else
                        {
                                VectorCopy( fl->light[style][j][0], light.light[0] );
                        }

                        LVector3 tmp;
                        LVector2 mins, maxs;

                        LVector3 samp_pos;
                        VectorCopy( samp->pos, samp_pos );

                        LuxelToWorldSpace( l, samp->mins[0], samp->mins[1], tmp );
                        WorldToLuxelSpace( &lightinfo[rad->facenum], tmp, mins );
                        LuxelToWorldSpace( l, samp->maxs[0], samp->maxs[1], tmp );
                        WorldToLuxelSpace( &lightinfo[rad->facenum], tmp, maxs );

                        AddDirectToRadial( rad, samp_pos, mins, maxs,
                                           &light, neighbor_bump, neighbor_bump );
                }
        }
        

        return rad;
}

void PatchLightmapCoordRange( radial_t *rad, int patchidx, LVector2 &mins, LVector2 &maxs )
{
        mins.set( 1e30, 1e30 );
        maxs.set( -1e30, -1e30 );

        patch_t *patch = &g_patches[patchidx];
        Winding *w = patch->winding;

        if ( !w )
        {
                mins.set( 0, 0 );
                maxs.set( 0, 0 );
                return;
        }

        LVector2 coord;
        LVector3 world;
        for ( int i = 0; i < w->m_NumPoints; i++ )
        {
                VectorCopy( w->m_Points[i], world );
                WorldToLuxelSpace( &lightinfo[rad->facenum], world, coord );
                mins[0] = std::min( mins[0], coord[0] );
                maxs[0] = std::max( maxs[0], coord[0] );
                mins[1] = std::min( mins[1], coord[1] );
                maxs[1] = std::max( maxs[1], coord[1] );
        }
}

void AddBouncedToRadial( radial_t *rad, const LVector3 &pos, const LVector2 &mins,
                         const LVector2 &maxs, bumpsample_t *light, bool bumpmap,
                         bool neighbor_bumpmap )
{
        int s_min, s_max, t_min, t_max;
        int s, t;
        float ds, dt;
        float r;
        int n;
        LVector2 coord;

        // convert world pos into local lightmap texture coord
        WorldToLuxelSpace( &lightinfo[rad->facenum], pos, coord );

        float dists, distt;

        dists = ( maxs[0] - mins[0] );
        distt = ( maxs[1] - mins[1] );

        // patches less than a luxel in size could be mistakenly filters, so clamp.
        dists = std::max( 1.0f, dists );
        distt = std::max( 1.0f, distt );

        // find possible domain of patch influence
        s_min = (int)( coord[0] - dists * RADIAL_DIST );
        t_min = (int)( coord[1] - distt * RADIAL_DIST );
        s_max = (int)( coord[0] + dists * RADIAL_DIST + 1.0 );
        t_max = (int)( coord[1] + distt * RADIAL_DIST + 1.0 );

        // clamp to valid luxel
        s_min = std::max( s_min, 0 );
        t_min = std::max( t_min, 0 );
        s_max = std::min( s_max, rad->w );
        t_max = std::min( t_max, rad->h );

        for ( s = s_min; s < s_max; s++ )
        {
                for ( t = t_min; t < t_max; t++ )
                {
                        // patch influence is based on patch size
                        ds = ( coord[0] - s ) / dists;
                        dt = ( coord[1] - t ) / distt;

                        r = RADIAL_DIST_2 - ( ds * ds + dt * dt );

                        int i = s + t * rad->w;

                        if ( r > 0.0 )
                        {
                                if ( bumpmap )
                                {
                                        if ( neighbor_bumpmap )
                                        {
                                                for ( n = 0; n < NUM_BUMP_VECTS + 1; n++ )
                                                {
                                                        rad->light[i].light[n].AddWeighted( light->light[n], r );
                                                }
                                        }
                                        else
                                        {
                                                rad->light[i].light[0].AddWeighted( light->light[0], r );
                                                for ( n = 1; n < NUM_BUMP_VECTS + 1; n++ )
                                                {
                                                        rad->light[i].light[n].AddWeighted( light->light[0], r * OO_SQRT_3 );
                                                }
                                        }
                                }
                                else
                                {
                                        rad->light[i].light[0].AddWeighted( light->light[0], r );
                                }

                                rad->weight[i] += r;
                        }
                }
        }
}

radial_t *BuildPatchRadial( int facenum )
{
        facelight_t *fl = &facelight[facenum];
        bool needs_bumpmap = fl->bumped;
        radial_t *rad = AllocRadial( facenum );
        faceneighbor_t *fn = &faceneighbor[facenum];

        LVector2 mins, maxs;

        patch_t *nextpatch;

        if ( g_face_patches[facenum] != -1 )
        {
                for ( patch_t *patch = &g_patches[g_face_patches[facenum]]; patch; patch = nextpatch )
                {
                        
                        nextpatch = nullptr;
                        if ( patch->next != -1 )
                        {
                                nextpatch = &g_patches[patch->next];
                        }

                        int patchidx = patch - g_patches.data();
                        PatchLightmapCoordRange( rad, patchidx, mins, maxs );

                        LVector3 patch_org;
                        VectorCopy( patch->origin, patch_org );
                        AddBouncedToRadial( rad, patch_org, mins, maxs,
                                            &patch->totallight, needs_bumpmap, needs_bumpmap );

                }
        }

        
        for ( int i = 0; i < fn->numneighbors; i++ )
        {
                int fn_idx = fn->neighbor[i];
                if ( g_face_patches[fn_idx] != -1 )
                {
                        for ( patch_t *patch = &g_patches[g_face_patches[fn_idx]]; patch; patch = nextpatch )
                        {
                                nextpatch = nullptr;
                                if ( patch->next != -1 )
                                {
                                        nextpatch = &g_patches[patch->next];
                                }

                                int patchidx = patch - g_patches.data();
                                PatchLightmapCoordRange( rad, patchidx, mins, maxs );

                                bool neighbor_bump = facelight[fn_idx].bumped;

                                LVector3 patch_org;
                                VectorCopy( patch->origin, patch_org );
                                AddBouncedToRadial( rad, patch_org, mins, maxs,
                                                    &patch->totallight, needs_bumpmap,
                                                    neighbor_bump );
                        }
                }
        }
        

        return rad;
}

bool SampleRadial( radial_t *rad, const LVector3 &pos,
                   bumpsample_t *sample, int bump_sample_count )
{
        int n;
        LVector2 coord;

        WorldToLuxelSpace( &lightinfo[rad->facenum], pos, coord );
        int u = (int)( coord[0] + 0.5f );
        int v = (int)( coord[1] + 0.5f );
        int i = u + v * rad->w;

        if ( u < 0 || u > rad->w || v < 0 || v > rad->h )
        {
                static bool warning = false;
                if ( true )
                {
                        Warning( "SampleRadial: Punting, waiting for fix\n" );
                        warning = true;
                }

                for ( n = 0; n < bump_sample_count; n++ )
                {
                        sample->light[n].light.set( 2550, 0, 0 );
                }

                return false;
        }

        bool base_sample_ok = true;
        for ( n = 0; n < bump_sample_count; n++ )
        {
                sample->light[n].Zero();

                if ( rad->weight[i] > WEIGHT_EPS )
                {
                        sample->light[n] = rad->light[i].light[n];
                        sample->light[n].Scale( 1.0 / rad->weight[i] );
                }
                else
                {
                        // error, luxel has no samples
                        sample->light[n].light.set( 0.0, 0.0, 0.0 );
                        if ( n == 0 )
                        {
                                base_sample_ok = false;
                        }
                }

               
        }

        return base_sample_ok;
}

// =====================================================================================
//  FinalLightFace
//      Add the indirect lighting on top of the direct lighting and save into final map format
// =====================================================================================
void FinalLightFace( const int facenum )
{
        dface_t *f;
        int i, j, k;
        facelight_t *fl;
        float minlight;
        int lightstyles;
        bumpsample_t lb, v;
        int bump_sample;
        radial_t *rad = nullptr;
        radial_t *prad = nullptr;

        f = g_bspdata->dfaces + facenum;

        // test for non-lit texture
        if ( g_bspdata->texinfo[f->texinfo].flags & TEX_SPECIAL )
        {
                return;
        }

        fl = &facelight[facenum];

        for ( lightstyles = 0; lightstyles < MAXLIGHTMAPS; lightstyles++ )
        {
                if ( f->styles[lightstyles] == 0xFF )
                {
                        break;
                }
        }

        if ( !lightstyles )
        {
                return;
        }
        
        //
        // sample the triangulation
        //
        minlight = FloatForKey( g_face_entity[facenum], "_minlight" ) * 128;

        bool needs_bumpmap = fl->bumped;
        int bump_sample_count = needs_bumpmap ? NUM_BUMP_VECTS + 1 : 1;

        for ( k = 0; k < lightstyles; k++ )
        {
                if ( !g_fastmode )
                {
                        rad = BuildLuxelRadial( facenum, k );
                }
                
                if ( g_numbounce > 0 && k == 0 )
                {
                        prad = BuildPatchRadial( facenum );
                }

                // compute average luxel color, but not for the bump samples
                LVector3 avg( 0.0 );
                int avg_count = 0;

                for ( j = 0; j < fl->numluxels; j++ )
                {
                        // direct lighting
                        bool base_sample_ok = true;

                        LVector3 samp_pos;
                        VectorCopy( fl->luxel_points[j], samp_pos );

                        if ( !g_fastmode )
                        {
                                base_sample_ok = SampleRadial( rad, samp_pos, &lb, bump_sample_count );

                        }
                        else
                        {
                                for ( int bump = 0; bump < bump_sample_count; bump++ )
                                {
                                        lb[bump] = fl->light[0][j].light[bump];
                                }
                        }

                        if ( prad )
                        {
                                // bounced light
                                // v is indirect light that is received on the luxel
                                SampleRadial( prad, samp_pos, &v, bump_sample_count );

                                //for ( bump_sample = 0; bump_sample < bump_sample_count; bump_sample++ )
                                //{
                                //        lb[bump_sample].AddLight( v[bump_sample] );
                                //}
                        }

                        for ( bump_sample = 0; bump_sample < bump_sample_count; bump_sample++ )
                        {
                                // clip from the bottom first
                                for ( i = 0; i < 3; i++ )
                                {
                                        lb[bump_sample][i] = std::max( lb[bump_sample][i], minlight );
                                        v[bump_sample][i] = std::max( v[bump_sample][i], minlight );
                                }

                                // save out to BSP file

                                // direct
                                colorrgbexp32_t *dcol = SampleLightmap( g_bspdata, g_bspdata->dfaces + facenum, j, k, bump_sample );
                                LVector3 dvcol;
                                VectorCopy( lb[bump_sample], dvcol );
                                VectorToColorRGBExp32( dvcol, *dcol );

                                // bounced
                                colorrgbexp32_t *bcol = SampleBouncedLightmap( g_bspdata, g_bspdata->dfaces + facenum, j );
                                LVector3 bvcol;
                                VectorCopy( v[bump_sample], bvcol );
                                VectorToColorRGBExp32( bvcol, *bcol );
                        }
                }

                FreeRadial( rad );
                rad = nullptr;
                FreeRadial( prad );
                prad = nullptr;
        }
}