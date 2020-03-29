#if 1

#include "leaf_ambient_lighting.h"
#include "qrad.h"
#include "anorms.h"
#include "lightingutils.h"
#include "lights.h"
#include "trace.h"

#include <aa_luse.h>
#include <randomizer.h>
#include <plane.h>

using std::min;
using std::max;

static LVector3i box_directions[6] = {
        LVector3i( 1,  0,  0 ),
        LVector3i( -1,  0,  0 ),
        LVector3i( 0,  1,  0 ),
        LVector3i( 0, -1,  0 ),
        LVector3i( 0,  0,  1 ),
        LVector3i( 0,  0, -1 )
};

struct AmbientSample
{
        LVector3 pos;
        LVector3 cube[6];
};

typedef pvector<AmbientSample> vector_ambientsample;
typedef pvector<dplane_t> vector_dplane;
pvector<vector_ambientsample> leaf_ambient_samples;

class LeafSampler
{
public:
        LeafSampler( int thread ) :
                _thread( thread )
        {
        }

        bool cast_ray_in_leaf( const LVector3 &start, const LVector3 &end,
                               int leaf_id, float *fraction, LNormalf &normal )
        {
                fraction[0] = 1.0;

                Trace trace;
                if ( trace_leaf_brushes( leaf_id, start, end, trace ) != 1.0 )
                {
                        fraction[0] = trace.fraction;
                        normal[0] = trace.plane.normal[0];
                        normal[1] = trace.plane.normal[1];
                        normal[2] = trace.plane.normal[2];
                }
                else
                {
                        nassertr( !trace.start_solid && !trace.all_solid, false );
                }

                return fraction[0] != 1.0;
        }

        void generate_leaf_sample_position( int leaf_id, const vector_dplane &leaf_planes,
                                            LVector3 &sample_pos )
        {
                dleaf_t *leaf = &g_bspdata->dleafs[leaf_id];

                float dx = leaf->maxs[0] - leaf->mins[0];
                float dy = leaf->maxs[1] - leaf->mins[1];
                float dz = leaf->maxs[2] - leaf->mins[2];

                bool valid = false;
                for ( int i = 0; i < 1000 && !valid; i++ )
                {
                        sample_pos[0] = leaf->mins[0] + _random.random_real( dx );
                        sample_pos[1] = leaf->mins[1] + _random.random_real( dy );
                        sample_pos[2] = leaf->mins[2] + _random.random_real( dz );
                        valid = true;
                        for ( int j = (int)leaf_planes.size() - 1; j >= 0 && valid; j-- )
                        {
                                float dist = DotProduct( leaf_planes[j].normal, sample_pos ) - leaf_planes[j].dist;
                                if ( dist < DIST_EPSILON )
                                {
                                        // Not inside the leaf, try again.
                                        valid = false;
                                        break;
                                }
                        }

                        if ( !valid )
                        {
                                continue;
                        }

                        for ( int j = 0; j < 6; j++ )
                        {
                                LVector3 start = sample_pos;
                                int axis = j % 3;
                                start[axis] = ( j < 3 ) ? leaf->mins[axis] : leaf->maxs[axis];
                                float t;
                                LNormalf normal;
                                cast_ray_in_leaf( sample_pos, start, leaf_id, &t, normal );
                                if ( t == 0.0 )
                                {
                                        valid = false;
                                        break;
                                }

                                if ( t != 1.0 )
                                {
                                        LVector3 delta = start - sample_pos;
                                        if ( DotProduct(delta, normal) > 0 )
                                        {
                                                valid = false;
                                                break;
                                        }
                                }
                        }
                }

                if ( !valid )
                {
                        // Didn't generate a valid sample point, just use the center of the leaf bbox
                        sample_pos = ( LPoint3( leaf->mins[0], leaf->mins[1], leaf->mins[2] ) + LPoint3( leaf->maxs[0], leaf->maxs[1], leaf->maxs[2] ) ) / 2.0;
                }
        }

private:
        int _thread;
        Randomizer _random;
};

bool is_leaf_ambient_surface_light( directlight_t *dl )
{
        static const float directlight_min_emit_surface = 0.005f;
        static const float directlight_min_emit_surface_distance_ratio = inv_r_squared( LVector3( 0, 0, 512 ) );

        if ( dl->type != emit_surface )
        {
                return false;
        }

        if ( dl->style != 0 )
        {
                return false;
        }

        PN_stdfloat intensity = max( dl->intensity[0], dl->intensity[1] );
        intensity = max( intensity, dl->intensity[2] );

        return ( intensity * directlight_min_emit_surface_distance_ratio ) < directlight_min_emit_surface;
}

void get_leaf_boundary_planes( vector_dplane &list, int leaf_id )
{
        list.clear();
        int node_id = leafparents[leaf_id];
        int child = -( leaf_id + 1 );

        while ( node_id >= 0 )
        {
                dnode_t *node = &g_bspdata->dnodes[node_id];
                dplane_t *node_plane = &g_bspdata->dplanes[node->planenum];
                if ( node->children[0] == child )
                {
                        // Front side
                        list.push_back( *node_plane );
                }
                else
                {
                        // Back side
                        dplane_t plane;
                        plane.dist = -node_plane->dist;
                        plane.normal[0] = -node_plane->normal[0];
                        plane.normal[1] = -node_plane->normal[1];
                        plane.normal[2] = -node_plane->normal[2];
                        plane.type = node_plane->type;
                        list.push_back( plane );
                }
                child = node_id;
                node_id = nodeparents[child];
        }
}

float worldlight_angle( const directlight_t *dl, const LVector3 &lnormal,
                        const LVector3 &snormal, const LVector3 &delta )
{
        float dot, dot2;
        if ( dl->type != emit_surface )
        {
                return 1.0;
        }

        dot = DotProduct( snormal, delta );
        if ( dot < 0 )
        {
                return 0;
        }

        dot2 = -DotProduct( delta, lnormal );
        if ( dot2 <= ON_EPSILON / 10 )
        {
                return 0; // behind light surface
        }

        return dot * dot2;
}

float worldlight_distance_falloff( const directlight_t *dl, const vec3_t &delta )
{
        if ( dl->type != emit_surface )
        {
                return 1.0;
        }

        LVector3 vec;
        VectorCopy( delta, vec );
        return inv_r_squared( vec );
}

void add_emit_surface_lights( const LVector3 &start, LVector3 *cube )
{
        float fraction_visible[4];
        memset( fraction_visible, 0, sizeof( fraction_visible ) );

        vec3_t vstart;
        VectorCopy( start, vstart );

        for ( int i = 0; i < Lights::numdlights; i++ )
        {
                directlight_t *dl = Lights::directlights[i];

                if ( dl == nullptr )
                {
                        continue;
                }

                if ( !( dl->flags & DLF_in_ambient_cube ) )
                {
                        continue;
                }

                if ( dl->type != emit_surface )
                {
                        continue;
                }

                vec3_t vorg;
                VectorCopy( dl->origin, vorg );

                if ( RADTrace::test_line( vstart, vorg ) == CONTENTS_SOLID )
                {
                        continue;
                }

                // Add this light's contribution.
                vec3_t delta;
                VectorSubtract( dl->origin, vstart, delta );
                float distance_scale = worldlight_distance_falloff( dl, delta );

                LVector3 deltanorm;
                VectorCopy( delta, deltanorm );
                deltanorm.normalize();
                LVector3 dlnorm;
                VectorCopy( dl->normal, dlnorm );
                float angle_scale = worldlight_angle( dl, dlnorm, deltanorm, deltanorm );

                float ratio = distance_scale * angle_scale * 1.0;
                if ( ratio == 0 )
                {
                        continue;
                }

                for ( int i = 0; i < 6; i++ )
                {
                        float t = DotProduct( box_directions[i], deltanorm );
                        if ( t > 0 )
                        {
                                LVector3 intensity;
                                VectorCopy( dl->intensity, intensity );
                                intensity *= ( t * ratio );
                                VectorAdd( cube[i], intensity, cube[i] );
                        }
                }
        }
}

void compute_ambient_from_spherical_samples( int thread, const LVector3 &sample_pos,
                                             LVector3 *cube )
{
        LVector3 radcolor[NUMVERTEXNORMALS];
        fltx4 tan_theta = ReplicateX4( std::tan( VERTEXNORMAL_CONE_INNER_ANGLE ) );

        int num_samples = NUMVERTEXNORMALS;
        int groups = ( num_samples & 0x3 ) ? ( num_samples / 4 ) + 1 : ( num_samples / 4 );

        LVector3 e[4];

        FourVectors start;
        start.DuplicateVector( sample_pos );

        for ( int i = 0; i < groups; i++ )
        {
                int nsample = 4 * i;
                int group_samples = std::min( 4, num_samples - nsample );

                FourVectors end;

                for ( int j = 0; j < 4; j++ )
                {
                        LVector3 anorm = ( j < group_samples ) ? g_anorms[nsample + j] : g_anorms[nsample + group_samples - 1];
                        e[j] = sample_pos + anorm * ( COORD_EXTENT * 1.74 );
                }

                end.LoadAndSwizzle( e[0], e[1], e[2], e[3] );

                LVector3 light_style_colors[4][MAX_LIGHTSTYLES];
                memset( light_style_colors, 0, sizeof( LVector3 ) * 4 * MAX_LIGHTSTYLES );

                calc_ray_ambient_lighting_SSE( thread, start, end, tan_theta, light_style_colors );

                for ( int j = 0; j < 4; j++ )
                {
                        radcolor[nsample + j] = light_style_colors[j][0];
                }
                
        }

        // accumulate samples into radiant box
        for ( int j = 5; j >= 0; j-- )
        {
                float t = 0;
                cube[j].set( 0, 0, 0 );
                for ( int i = 0; i < NUMVERTEXNORMALS; i++ )
                {
                        float c = DotProduct( g_anorms[i], box_directions[j] );
                        if ( c > 0 )
                        {
                                t += c;
                                cube[j] += radcolor[i] * c;
                        }
                }

                cube[j] *= ( 1 / t );
        }

        // Now add direct light from the emit_surface lights. These go in the ambient cube because
        // there are a ton of them and they are often so dim that they get filtered out by r_worldlightmin.
        add_emit_surface_lights( sample_pos, cube );
}

void add_sample_to_list( vector_ambientsample &list, const LVector3 &sample_pos, LVector3 *cube )
{
        const size_t max_samples = 16;

        AmbientSample sample;
        sample.pos = sample_pos;
        for ( int i = 0; i < 6; i++ )
        {
                sample.cube[i] = cube[i];
        }

        list.push_back( sample );

        if ( list.size() <= max_samples )
        {
                return;
        }

        int nearest_neighbor_idx = 0;
        float nearest_neighbor_dist = FLT_MAX;
        float nearest_neighbor_total = 0;
        for ( int i = 0; i < list.size(); i++ )
        {
                int closest_idx = 0;
                float closest_dist = FLT_MAX;
                float total_dc = 0;
                for ( int j = 0; j < list.size(); j++ )
                {
                        if ( j == i )
                        {
                                continue;
                        }

                        float dist = ( list[i].pos - list[j].pos ).length();
                        float max_dc = 0;
                        for ( int k = 0; k < 6; k++ )
                        {
                                // color delta is computed per-component, per cube side
                                for ( int s = 0; s < 3; s++ )
                                {
                                        float dc = std::fabs( list[i].cube[k][s] - list[j].cube[k][s] );
                                        max_dc = max( max_dc, dc );
                                }
                                total_dc += max_dc;
                        }

                        // need a measurable difference in color or we'll just rely on position
                        if ( max_dc < 1e-4f )
                        {
                                max_dc = 0;

                        }
                        else if ( max_dc > 1.0f )
                        {
                                max_dc = 1.0f;
                        }

                        float distance_factor = 0.1f + ( max_dc * 0.9f );
                        dist *= distance_factor;

                        // find the "closest" sample to this one
                        if ( dist < closest_dist )
                        {
                                closest_dist = dist;
                                closest_idx = j;
                        }
                }

                // the sample with the "closest" neighbor is rejected
                if ( closest_dist < nearest_neighbor_dist || ( closest_dist == nearest_neighbor_dist && total_dc < nearest_neighbor_total ) )
                {
                        nearest_neighbor_dist = closest_dist;
                        nearest_neighbor_idx = i;
                }
        }

        list.erase( list.begin() + nearest_neighbor_idx );
}

void compute_ambient_for_leaf( int thread, int leaf_id,
                               vector_ambientsample &list )
{
        if ( g_bspdata->dleafs[leaf_id].contents == CONTENTS_SOLID )
        {
                // Don't generate any samples in solid leaves.
                // NOTE: We copy the nearest non-solid leaf sample pointers into this leaf at the end.
                std::cout << "Leaf " << leaf_id << " is solid" << std::endl;
                return;
        }

        vector_dplane leaf_planes;
        LeafSampler sampler( thread );
        get_leaf_boundary_planes( leaf_planes, leaf_id );
        list.clear();

        int xsize = ( g_bspdata->dleafs[leaf_id].maxs[0] - g_bspdata->dleafs[leaf_id].mins[0] ) / 32;
        int ysize = ( g_bspdata->dleafs[leaf_id].maxs[1] - g_bspdata->dleafs[leaf_id].mins[1] ) / 32;
        int zsize = ( g_bspdata->dleafs[leaf_id].maxs[2] - g_bspdata->dleafs[leaf_id].mins[2] ) / 64;
        xsize = max( xsize, 1 );
        ysize = max( ysize, 1 );
        zsize = max( zsize, 1 );

        int volume_count = xsize * ysize * zsize;
        // Don't do any more than 128 samples
        int sample_count = clamp( volume_count, 1, 128 );
        LVector3 cube[6];
        for ( int i = 0; i < sample_count; i++ )
        {
                LVector3 sample_pos;
                sampler.generate_leaf_sample_position( leaf_id, leaf_planes, sample_pos );
                compute_ambient_from_spherical_samples( thread, sample_pos, cube );
                add_sample_to_list( list, sample_pos, cube );
        }
}

static void ComputeLeafAmbientLighting( int thread )
{
        while ( true )
        {
                int leaf_id = GetThreadWork();
                if ( leaf_id == -1 )
                {
                        break;
                }

                vector_ambientsample list;
                compute_ambient_for_leaf( thread, leaf_id, list );

                leaf_ambient_samples[leaf_id].resize( list.size() );
                for ( size_t i = 0; i < list.size(); i++ )
                {
                        leaf_ambient_samples[leaf_id][i] = list[i];
                }
        }
}

void LeafAmbientLighting::
compute_per_leaf_ambient_lighting()
{
        // Figure out which ambient lights should go in the per-leaf ambient cubes.
        int in_ambient_cube = 0;
        int surface_lights = 0;

        for ( int i = 0; i < Lights::numdlights; i++ )
        {
                directlight_t *dl = Lights::directlights[i];

                if ( dl == nullptr )
                {
                        continue;
                }

                if ( is_leaf_ambient_surface_light( dl ) )
                {
                        dl->flags |= DLF_in_ambient_cube;

                }
                else
                {
                        dl->flags &= ~DLF_in_ambient_cube;
                }

                if ( dl->type == emit_surface )
                {
                        surface_lights++;
                }

                if ( dl->flags & DLF_in_ambient_cube )
                {
                        in_ambient_cube++;
                }
        }

        int numleafs = g_bspdata->dmodels[0].visleafs + 1;

        leaf_ambient_samples.resize( numleafs );

        NamedRunThreadsOn( numleafs, g_estimate, ComputeLeafAmbientLighting );

        // now write out the data :)
        g_bspdata->leafambientindex.clear();
        g_bspdata->leafambientlighting.clear();
        g_bspdata->leafambientindex.resize( numleafs );
        g_bspdata->leafambientlighting.reserve( numleafs * 4 );
        for ( int leaf_id = 0; leaf_id < numleafs; leaf_id++ )
        {
                const vector_ambientsample &list = leaf_ambient_samples[leaf_id];
                g_bspdata->leafambientindex[leaf_id].num_ambient_samples = list.size();

                if ( list.size() == 0 )
                {
                        g_bspdata->leafambientindex[leaf_id].first_ambient_sample = 0;
                }
                else
                {
                        g_bspdata->leafambientindex[leaf_id].first_ambient_sample = g_bspdata->leafambientlighting.size();

                        for ( int i = 0; i < list.size(); i++ )
                        {
                                dleafambientlighting_t light;
                                light.x = fixed_8_fraction( list[i].pos[0], g_bspdata->dleafs[leaf_id].mins[0], g_bspdata->dleafs[leaf_id].maxs[0] );
                                light.y = fixed_8_fraction( list[i].pos[1], g_bspdata->dleafs[leaf_id].mins[1], g_bspdata->dleafs[leaf_id].maxs[1] );
                                light.z = fixed_8_fraction( list[i].pos[2], g_bspdata->dleafs[leaf_id].mins[2], g_bspdata->dleafs[leaf_id].maxs[2] );
                                light.pad = 0;
                                for ( int side = 0; side < 6; side++ )
                                {
                                        LRGBColor temp;
                                        VectorCopy( list[i].cube[side], temp );
                                        VectorToColorRGBExp32( list[i].cube[side], light.cube.color[side] );
                                }

                                g_bspdata->leafambientlighting.push_back( light );
                        }
                }
        }

        
        for ( int i = 0; i < numleafs; i++ )
        {
                if ( g_bspdata->leafambientindex[i].num_ambient_samples == 0 )
                {
                        if ( g_bspdata->dleafs[i].contents == CONTENTS_SOLID )
                        {
                                Warning( "Bad leaf ambient for leaf %d\n", i );
                        }

                        //int ret_leaf = nearest_neighbor_with_light( i );
                        //g_leafambientindex[i].num_ambient_samples = 0;
                        //g_leafambientindex[i].first_ambient_sample = ret_leaf;
                }
        }
        
}

#endif