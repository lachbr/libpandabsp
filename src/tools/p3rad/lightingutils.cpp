/**
 * PANDA3D BSP TOOLS
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file lightingutils.cpp
 * @author Brian Lach
 * @date July 31, 2018
 *
 */

#include "lightingutils.h"
#include "winding.h"
#include "anorms.h"
#include "halton.h"

#include "lights.h"

#include "lightmap.h"

#include "trace.h"

#define NEVER_UPDATED -9999

static bool test_point_against_surface( const LVector3 &point, dface_t *face, texinfo_t *tex, LTexCoordf &luxel_coord )
{
        // Specials don't have lightmaps
        if ( tex->flags & TEX_SPECIAL )
        {
                return false;
        }

        float s, t;
        s = DotProduct( point, tex->lightmap_vecs[0] ) + tex->lightmap_vecs[0][3];
        t = DotProduct( point, tex->lightmap_vecs[1] ) + tex->lightmap_vecs[1][3];

        if ( s < face->lightmap_mins[0] || t < face->lightmap_mins[1] )
        {
                // Not in bounds of lightmap
                return false;
        }

        float ds = s - face->lightmap_mins[0];
        float dt = t - face->lightmap_mins[1];
        if ( ds > face->lightmap_size[0] || dt > face->lightmap_size[1] )
        {
                // Doesn't lie in rectangle.
                return false;
        }

        luxel_coord.set( ds, dt );
        return true;
}

void clip_box_to_brush( Trace *trace, const LPoint3 &mins, const LPoint3 &maxs,
                        const LPoint3 &p1, const LPoint3 &p2, dbrush_t *brush )
{
        dplane_t *plane, *clip_plane;
        float dist;
        LVector3 ofs;
        float d1, d2;
        float f;
        dbrushside_t *side, *leadside;

        if ( !brush->numsides )
        {
                return;
        }

        float enter_frac = NEVER_UPDATED;
        float leave_frac = 1.0;
        clip_plane = nullptr;

        bool getout = false;
        bool startout = false;
        leadside = nullptr;

        for ( int i = 0; i < brush->numsides; i++ )
        {
                side = &g_bspdata->dbrushsides[brush->firstside + i];
                plane = &g_bspdata->dplanes[side->planenum];

                if ( !trace->is_point )
                {
                        ofs[0] = ( plane->normal[0] < 0 ) ? maxs[0] : mins[0];
                        ofs[1] = ( plane->normal[1] < 0 ) ? maxs[1] : mins[1];
                        ofs[2] = ( plane->normal[2] < 0 ) ? maxs[2] : mins[2];
                        dist = DotProduct( ofs, plane->normal );
                        dist = plane->dist - dist;

                }
                else
                {
                        if ( side->bevel == 1 )
                        {
                                continue;
                        }

                        dist = plane->dist;
                }

                d1 = DotProduct( p1, plane->normal ) - dist;
                d2 = DotProduct( p2, plane->normal ) - dist;

                if ( d1 > 0 && d2 > 0 )
                {
                        return;
                }

                if ( d2 > 0 )
                {
                        getout = true;
                }
                if ( d1 > 0 )
                {
                        startout = true;
                }

                if ( d1 <= 0 && d2 <= 0 )
                {
                        continue;
                }

                if ( d1 > d2 )
                {
                        f = ( d1 - DIST_EPSILON ) / ( d1 - d2 );
                        if ( f > enter_frac )
                        {
                                enter_frac = f;
                                clip_plane = plane;
                                leadside = side;
                        }

                }
                else
                {
                        f = ( d1 + DIST_EPSILON ) / ( d1 - d2 );
                        if ( f < leave_frac )
                        {
                                leave_frac = f;
                        }
                }
        }

        if ( !startout )
        {
                trace->start_solid = true;
                if ( !getout )
                {
                        trace->all_solid = true;
                }

                return;
        }

        if ( enter_frac < leave_frac )
        {
                if ( enter_frac > NEVER_UPDATED && enter_frac < trace->fraction )
                {
                        if ( enter_frac < 0 )
                        {
                                enter_frac = 0;
                        }

                        trace->fraction = enter_frac;
                        trace->plane.dist = clip_plane->dist;
                        VectorCopy( clip_plane->normal, trace->plane.normal );
                        trace->plane.type = clip_plane->type;
                        if ( leadside->texinfo != -1 )
                        {
                                trace->surface = &g_bspdata->texinfo[leadside->texinfo];
                        }
                        else
                        {
                                trace->surface = 0;
                        }
                        trace->contents = brush->contents;
                }
        }
}

float trace_leaf_brushes( int leaf_id, const LVector3 &start, const LVector3 &end, Trace &trace_out )
{
        dleaf_t *leaf = &g_bspdata->dleafs[leaf_id];
        Trace trace;
        memset( &trace, 0, sizeof( Trace ) );
        trace.is_point = true;
        trace.start_solid = false;
        trace.fraction = 1.0;

        for ( int i = 0; i < leaf->numleafbrushes; i++ )
        {
                int brushnum = g_bspdata->dleafbrushes[leaf->firstleafbrush + i];
                dbrush_t *b = &g_bspdata->dbrushes[brushnum];
                if ( b->contents != CONTENTS_SOLID )
                {
                        continue;
                }
                LVector3 extents( 0 );
                clip_box_to_brush( &trace, extents, extents, start, end, b );
                if ( trace.fraction != 1.0 || trace.start_solid )
                {
                        if ( trace.start_solid )
                        {
                                trace.fraction = 0.0;
                        }
                        trace_out = trace;
                        return trace.fraction;
                }
        }

        trace_out = trace;
        return 1.0;
}

static directlight_t *find_ambient_sky_light()
{
        static directlight_t *found_light = nullptr;

        // So we don't have to keep finding the same sky light
        if ( found_light == nullptr )
        {
                for ( directlight_t *dl = Lights::activelights; dl; dl = dl->next )
                {
                        if ( dl->type == emit_skyambient )
                        {
                                found_light = dl;
                                break;
                        }
                }
        }

        return found_light;
}

void compute_ambient_from_surface( dface_t *face, directlight_t *skylight,
                                   LRGBColor &color )
{
        texinfo_t *texinfo = &g_bspdata->texinfo[face->texinfo];
        if ( texinfo )
        {
                if ( texinfo->flags & TEX_SKY )
                {
                        if ( skylight )
                        {
                                // Add in sky ambient
                                VectorCopy( skylight->intensity, color );
                        }

                }
                else
                {
                        radtexture_t *rtex = g_textures + texinfo->texref;
                        VectorMultiply( color, rtex->reflectivity, color );
                }
        }
}

static void compute_lightmap_color_from_average( dface_t *face, directlight_t *skylight,
                                                 float scale, LVector3 *colors )
{
        texinfo_t *tex = &g_bspdata->texinfo[face->texinfo];
        if ( tex->flags & TEX_SKY )
        {
                if ( skylight )
                {
                        colors[0] += skylight->intensity * scale;
                }
                return;
        }

        for ( int maps = 0; maps < MAXLIGHTMAPS && face->styles[maps] != 0xFF; maps++ )
        {
                int style = face->styles[maps];
                ThreadLock();
                LRGBColor avg_color = dface_AvgLightColor( g_bspdata, face, style );
                ThreadUnlock();
                LRGBColor color = avg_color;

                compute_ambient_from_surface( face, skylight, color );  
                colors[style] += color * scale;
        }
}

void compute_lightmap_color_point_sample( dface_t *face, directlight_t *skylight, LTexCoordf &coord, float scale, LVector3 *colors )
{
        // Face unaffected by light
        if ( face->lightofs == -1 )
        {
                return;
        }

        int smax = face->lightmap_size[0] + 1;
        int tmax = face->lightmap_size[1] + 1;

        int ds = clamp( (int)coord[0], 0, smax - 1 );
        int dt = clamp( (int)coord[1], 0, tmax - 1 );

        int offset = smax * tmax;
        if ( face->bumped_lightmap )
        {
                offset *= ( NUM_BUMP_VECTS + 1 );
        }

        colorrgbexp32_t *lightmap = &g_bspdata->lightdata[face->lightofs];
        lightmap += dt * smax + ds;
        for ( int maps = 0; maps < MAXLIGHTMAPS && face->styles[maps] != 0xFF; maps++ )
        {
                int style = face->styles[maps];

                LVector3 color( 0 );
                ColorRGBExp32ToVector( *lightmap, color );

                compute_ambient_from_surface( face, skylight, color );
                colors[style] += color * scale;

                lightmap += offset;
        }
}

struct lightsurface_t
{
        bool has_luxel;
        LTexCoordf luxel_coord;
        dface_t *surface;
        float hit_fraction;

        lightsurface_t() :
                has_luxel( false ),
                surface( nullptr )
        {
        }
};

struct lightsurface_SSE_t
{
        u32x4 has_luxel;
        FourVectors dir;
        FourVectors delta;
        FourVectors luxel_coord;
        dface_t *surface[4];
        fltx4 hit_fraction;
};

static bool lightsurface_findintersection( const Ray &ray, lightsurface_t *surf )
{
        BitMask32 mask = CONTENTS_SOLID | CONTENTS_SKY;

        RayTraceHitResult result = RADTrace::scene->trace_line( ray.start, ray.end, mask );
        surf->hit_fraction = result.hit_fraction;
        if ( !result.hit )
        {
                return false;
        }

        LPoint3 pt;
        VectorMA( ray.start, result.hit_fraction, ray.delta.normalized(), pt );
        surf->surface = RADTrace::get_dface( result );
        if ( !surf->surface )
        {
                return false;
        }
        texinfo_t *tex = &g_bspdata->texinfo[surf->surface->texinfo];
        surf->has_luxel = false;
        surf->luxel_coord.set( 0, 0 );
        if ( !( tex->flags & TEX_SKY ) )
        {
                if ( test_point_against_surface( pt, surf->surface, tex, surf->luxel_coord ) )
                {
                        surf->has_luxel = true;
                }
        }

        return true;
}

static void lightsurface_findintersection_SSE( const FourVectors &start, const FourVectors &end, lightsurface_SSE_t *surf )
{
        u32x4 mask = ReplicateIX4( CONTENTS_SOLID | CONTENTS_SKY );

        surf->delta = end;
        surf->delta -= start;

        surf->dir = surf->delta;
        surf->dir.VectorNormalize();

        RayTraceHitResult4 result;
        RADTrace::scene->trace_four_lines( start, end, mask, &result );
        surf->hit_fraction = result.hit_fraction;

        FourVectors point = surf->dir;
        point *= surf->hit_fraction;
        point += start;

        surf->has_luxel = Four_Zeros;

        for ( int i = 0; i < 4; i++ )
        {

                if ( surf->hit_fraction.m128_f32[i] < 1.0 - EQUAL_EPSILON ) // did we hit something?
                {
                        int geomidx = RADTrace::dface_lookup.find( result.geom_id.m128_u32[i] );
                        if ( geomidx != -1 )
                        {
                                surf->surface[i] = RADTrace::dface_lookup.get_data( geomidx );
                                if ( surf->surface[i] != nullptr )
                                {
                                        texinfo_t *tex = &g_bspdata->texinfo[surf->surface[i]->texinfo];
                                        if ( !( tex->flags & TEX_SKY ) )
                                        {
                                                LTexCoordf coord;
                                                if ( test_point_against_surface( point.Vec(i), surf->surface[i], tex, coord ) )
                                                {
                                                        surf->has_luxel = SetComponentSIMD( surf->has_luxel, i, 1.0 );
                                                        surf->luxel_coord.x = SetComponentSIMD( surf->luxel_coord.x, i, coord[0] );
                                                        surf->luxel_coord.y = SetComponentSIMD( surf->luxel_coord.y, i, coord[1] );
                                                }
                                        }
                                }
                        }
                        else
                        {
                                surf->surface[i] = nullptr;
                        }
                }
                else
                {
                        surf->surface[i] = nullptr;
                }
        }
}

void calc_ray_ambient_lighting_SSE( int thread, const FourVectors &start,
                                const FourVectors &end, fltx4 tan_theta,
                                LVector3 color[4][MAX_LIGHTSTYLES] )
{
        directlight_t *skylight = find_ambient_sky_light();

        lightsurface_SSE_t surf;
        lightsurface_findintersection_SSE( start, end, &surf );

        // compute the approximate radius of a circle centered around the intersection point
        fltx4 dist = surf.delta.length();
        dist = MulSIMD( dist, tan_theta );
        dist = MulSIMD( dist, surf.hit_fraction );

        // until 20" we use the point sample, then blend in the average until we're covering 40"
        // This is attempting to model the ray as a cone - in the ideal case we'd simply sample all
        // luxels in the intersection of the cone with the surface.  Since we don't have surface 
        // neighbor information computed we'll just approximate that sampling with a blend between
        // a point sample and the face average.
        // This yields results that are similar in that aliasing is reduced at distance while 
        // point samples provide accuracy for intersections with near geometry
        fltx4 scale_avg;
        for ( int i = 0; i < 4; i++ )
        {
                if ( !surf.surface[i] )
                        continue;
                
                if ( surf.has_luxel.m128_u32[i] == 0 )
                {
                        scale_avg = SetComponentSIMD( scale_avg, i, 1.0 );
                }
                else
                {
                        scale_avg = SetComponentSIMD( scale_avg, i, RemapValClamped( scale_avg.m128_f32[i], 20, 40, 0.0, 1.0 ) );
                }
                
        }

        fltx4 scale_sample = SubSIMD( Four_Ones, scale_avg );

        for ( int i = 0; i < 4; i++ )
        {
                if ( !surf.surface[i] )
                        continue;

                if ( scale_avg.m128_f32[i] != 0 )
                {
                        compute_lightmap_color_from_average( surf.surface[i], skylight, scale_avg.m128_f32[i], color[i] );
                }
                if ( scale_sample.m128_f32[i] != 0 )
                {
                        LTexCoordf coord( surf.luxel_coord.x.m128_f32[i],
                                          surf.luxel_coord.y.m128_f32[i] );
                        compute_lightmap_color_point_sample( surf.surface[i], skylight, coord, scale_sample.m128_f32[i], color[i] );
                }

        }
        
}

void calc_ray_ambient_lighting( int thread, const LVector3 &start,
                                const LVector3 &end, float tan_theta,
                                LVector3 *color )
{
        directlight_t *skylight = find_ambient_sky_light();
        Ray ray( start, end, LVector3::zero(), LVector3::zero() );
        lightsurface_t surf;
        if ( !lightsurface_findintersection( ray, &surf ) )
                return;

        // compute the approximate radius of a circle centered around the intersection point
        float dist = ray.delta.length() * tan_theta * surf.hit_fraction;

        // until 20" we use the point sample, then blend in the average until we're covering 40"
        // This is attempting to model the ray as a cone - in the ideal case we'd simply sample all
        // luxels in the intersection of the cone with the surface.  Since we don't have surface 
        // neighbor information computed we'll just approximate that sampling with a blend between
        // a point sample and the face average.
        // This yields results that are similar in that aliasing is reduced at distance while 
        // point samples provide accuracy for intersections with near geometry
        float scale_avg = RemapValClamped( dist, 20, 40, 0.0f, 1.0f );

        if ( !surf.has_luxel )
        {
                scale_avg = 1.0;
        }

        float scale_sample = 1.0f - scale_avg;
        if ( scale_avg != 0 )
        {
                compute_lightmap_color_from_average( surf.surface, skylight, scale_avg, color );
        }
        if ( scale_sample != 0 )
        {
                compute_lightmap_color_point_sample( surf.surface, skylight, surf.luxel_coord, scale_sample, color );
        }
}

void ComputeIndirectLightingAtPoint( const LVector3 &vpos, const LNormalf &vnormal, LVector3 &color, bool ignore_normals )
{
        int samples = NUMVERTEXNORMALS;
        if ( g_fastmode )
        {
                samples /= 4;
        }

        int groups = ( samples & 0x3 ) ? ( samples / 4 ) + 1 : ( samples / 4 );

        LVector3 n[4];
        float dots[4];

        FourVectors vpos4;
        vpos4.DuplicateVector( vpos );

        PN_stdfloat total_dot = 0;
        DirectionalSampler_t sampler;
        for ( int j = 0; j < groups; j++ )
        {
                int nsample = 4 * j;
                int group_samples = std::min( 4, samples - nsample );
                
                memset( dots, 0, sizeof( float ) * 4 );
                memset( n, 0, sizeof( LVector3 ) * 4 );
                for ( int i = 0; i < group_samples; i++ )
                {
                        LVector3 sampling_normal = sampler.NextValue();
                        PN_stdfloat dot;
                        if ( ignore_normals )
                        {
                                dot = 0.7071 / 2;
                        }
                        else
                        {
                                dot = DotProduct( vnormal, sampling_normal );
                        }
                        dots[i] = dot;
                        if ( dot <= EQUAL_EPSILON )
                        {
                                // reject angles behind our plane
                                continue;
                        }
                        n[i] = sampling_normal;
                        total_dot += dot;
                }
                
                FourVectors normal;
                normal.LoadAndSwizzle( n[0], n[1], n[2], n[3] );

                // trace to determine surface
                FourVectors end = normal;
                end *= ReplicateX4( MAX_TRACE_LENGTH );
                end += vpos4;

                lightsurface_SSE_t surf;
                lightsurface_findintersection_SSE( vpos4, end, &surf );

                for ( int i = 0; i < group_samples; i++ )
                {
                        if ( !surf.surface[i] )
                                continue;

                        if ( dots[i] <= EQUAL_EPSILON )
                        {
                                // reject angles behind our plane
                                continue;
                        }

                        // get color from surface lightmap
                        texinfo_t *tex = &g_bspdata->texinfo[surf.surface[i]->texinfo];
                        if ( !tex || tex->flags & TEX_SPECIAL )
                        {
                                // ignore contribution from sky or non lit textures
                                // sky ambient already accounted for during direct pass
                                continue;
                        }

                        if ( surf.surface[i]->styles[0] == 255 || surf.surface[i]->lightofs == -1 )
                        {
                                // no light affects this face
                                continue;
                        }

                        LVector3 lightmap_col;
                        if ( surf.has_luxel.m128_u32[i] == 0 )
                        {
                                ThreadLock();
                                lightmap_col = dface_AvgLightColor( g_bspdata, surf.surface[i], 0 );
                                ThreadUnlock();
                        }
                        else
                        {
                                // get color from the luxel itself
                                int smax = surf.surface[i]->lightmap_size[0] + 1;
                                int tmax = surf.surface[i]->lightmap_size[1] + 1;

                                // luxelcoord is in the space of the accumulated lightmap page; we need to convert
                                // it to be in the space of the surface
                                int ds = clamp( (int)surf.luxel_coord.x.m128_f32[i], 0, smax - 1 );
                                int dt = clamp( (int)surf.luxel_coord.y.m128_f32[i], 0, tmax - 1 );

                                colorrgbexp32_t *lightmap = &g_bspdata->lightdata[surf.surface[i]->lightofs];
                                lightmap += dt * smax + ds;
                                ColorRGBExp32ToVector( *lightmap, lightmap_col );
                        }


                        VectorMultiply( lightmap_col, g_textures[tex->texref].reflectivity, lightmap_col );
                        VectorAdd( color, lightmap_col, color );
                }
                
        }

        if ( total_dot )
        {
                VectorScale( color, 1.0 / total_dot, color );
        }
}

void ComputeDirectLightingAtPoint( const LVector3 &vpos, const LNormalf &vnormal, LVector3 &color )
{
        SSE_sampleLightOutput_t output;
        int leaf = PointInLeaf( vpos ) - g_bspdata->dleafs;
        for ( directlight_t *dl = Lights::activelights; dl != nullptr; dl = dl->next )
        {
                // skip lights with style
                if ( dl->style )
                        continue;

                // is this light potentially visible?
                if ( !PVSCheck( dl->pvs, leaf ) )
                        continue;

                // push the vertex towards the light to avoid surface acne
                LVector3 adjusted = vpos;
                float epsilon = 0.0;
                if ( dl->type != emit_skyambient )
                {
                        // push towards the light
                        LVector3 fudge;
                        if ( dl->type == emit_skylight )
                        {
                                fudge = -( dl->normal );
                        }
                        else
                        {
                                fudge = dl->origin - vpos;
                                fudge.normalize();
                        }
                        fudge *= 4.0;
                        adjusted += fudge;
                }
                else
                {
                        adjusted += 4.0 * vnormal;
                }

                FourVectors adjusted4;
                FourVectors normal4;
                adjusted4.DuplicateVector( adjusted );
                normal4.DuplicateVector( vnormal );

                GatherSampleLightSSE( output, dl, -1, adjusted4, &normal4, 1, 0, 0, epsilon );

                VectorMA( color, output.falloff.m128_f32[0] * output.dot[0].m128_f32[0], dl->intensity, color );
        }
}