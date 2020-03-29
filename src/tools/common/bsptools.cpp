#include "bsptools.h"

BaseBSPEnumerator::BaseBSPEnumerator( bspdata_t *dat ) :
        data( dat )
{
}

bool r_enumerate_nodes_along_ray( int node_id, const Ray &ray, float start,
                                  float end, BaseBSPEnumerator *surf, int context,
                                  float scale )
{
        float front, back;
        float start_dot_n, delta_dot_n;

        while ( node_id >= 0 )
        {
                dnode_t *node = &surf->data->dnodes[node_id];
                dplane_t *plane = &surf->data->dplanes[node->planenum];

                if ( plane->type == plane_z )
                {
                        start_dot_n = ray.start[plane->type];
                        delta_dot_n = ray.delta[plane->type];
                }
                else
                {
                        start_dot_n = DotProduct( ray.start, plane->normal );
                        delta_dot_n = DotProduct( ray.delta, plane->normal );
                }

                front = start_dot_n + start * delta_dot_n - (plane->dist / scale);
                back = start_dot_n + end * delta_dot_n - (plane->dist / scale);

                if ( front <= -TEST_EPSILON && back <= -TEST_EPSILON )
                {
                        node_id = node->children[1];
                }
                else if ( front >= TEST_EPSILON && back >= TEST_EPSILON )
                {
                        node_id = node->children[0];
                }
                else
                {
                        // test the front side first
                        bool side = front < 0;

                        float split_frac;
                        if ( delta_dot_n == 0.0 )
                        {
                                split_frac = 1.0;
                        }
                        else
                        {
                                split_frac = ( (plane->dist / scale) - start_dot_n ) / delta_dot_n;
                                if ( split_frac < 0.0 )
                                {
                                        split_frac = 0.0;
                                }
                                else if ( split_frac > 1.0 )
                                {
                                        split_frac = 1.0;
                                }
                        }

                        bool r = r_enumerate_nodes_along_ray( node->children[side], ray, start,
                                                              split_frac, surf, context, scale );
                        if ( !r )
                        {
                                return r;
                        }

                        // Visit the node...
                        if ( !surf->enumerate_node( node_id, ray, split_frac, context ) )
                        {
                                return false;
                        }

                        return r_enumerate_nodes_along_ray( node->children[!side], ray, split_frac,
                                                            end, surf, context, scale );
                }
        }

        // Visit the leaf...
        return surf->enumerate_leaf( ~node_id, ray, start, end, context );
}

bool enumerate_nodes_along_ray( const Ray &ray, BaseBSPEnumerator *surf, int context, float scale )
{
        return r_enumerate_nodes_along_ray( 0, ray, 0.0, 1.0, surf, context, scale );
}

lightfalloffparams_t GetLightFalloffParams( entity_t *e, LVector3 &intensity )
{
        lightfalloffparams_t out;

        float d50 = FloatForKey( e, "_fifty_percent_distance" );
        out.start_fade_distance = 0;
        out.end_fade_distance = -1;
        out.cap_distance = 1.0e22;
        if ( d50 )
        {
                float d0 = FloatForKey( e, "_zero_percent_distance" );
                if ( d0 < d50 )
                {
                        printf( "Warning: light has _fifty_percent_distance of %f but _zero_percent_distance of %f\n", d50, d0 );
                        d0 = d50 * 2.0;
                }
                float a = 0, b = 1, c = 0;
                if ( !SolveInverseQuadraticMonotonic( 0, 1.0, d50, 2.0, d0, 256.0, a, b, c ) )
                {
                        printf( "Warning: Can't solve quadratic for light %f %f\n", d50, d0 );
                }

                // it it possible that the parameters couldn't be used because of enforing monoticity. If so, rescale so at
                // least the 50 percent value is right
                float v50 = c + d50 * ( b + d50 * a );
                float scale = 2.0 / v50;
                a *= scale;
                b *= scale;
                c *= scale;
                out.quadratic_atten = a;
                out.linear_atten = b;
                out.constant_atten = c;

                if ( IntForKey( e, "_hardfalloff" ) )
                {
                        out.end_fade_distance = d0;
                        out.start_fade_distance = 0.75 * d0 + 0.25 * d50;
                }
                else
                {
                        // now, we will find the point at which the 1/x term reaches its maximum value, and
                        // prevent the light from going past there. If a user specifes an extreme falloff, the
                        // quadratic will start making the light brighter at some distance. We handle this by
                        // fading it from the minimum brightess point down to zero at 10x the minimum distance
                        if ( std::fabs( a ) > 0.0 )
                        {
                                float flMax = b / ( -2.0 * a );				// where f' = 0
                                if ( flMax > 0.0 )
                                {
                                        out.cap_distance = flMax;
                                        out.start_fade_distance = flMax;
                                        out.end_fade_distance = 10.0 * flMax;
                                }
                        }
                }
        }
        else
        {
                out.constant_atten = FloatForKey( e, "_constant_attn" );
                out.linear_atten = FloatForKey( e, "_linear_attn" );
                out.quadratic_atten = FloatForKey( e, "_quadratic_attn" );

                out.radius = FloatForKey( e, "_distance" );

                // clamp values >= 0
                if ( out.constant_atten < EQUAL_EPSILON )
                        out.constant_atten = 0.0;
                if ( out.linear_atten < EQUAL_EPSILON )
                        out.linear_atten = 0.0;
                if ( out.quadratic_atten < EQUAL_EPSILON )
                        out.quadratic_atten = 0.0;
                if ( out.constant_atten < EQUAL_EPSILON &&
                     out.linear_atten < EQUAL_EPSILON &&
                     out.quadratic_atten < EQUAL_EPSILON )
                        out.constant_atten = 1;

                // scale intensity for unit 100 distance
                float ratio = ( out.constant_atten + 100 * out.linear_atten + 100 * 100 * out.quadratic_atten );
                if ( ratio > 0 )
                {
                        VectorScale( intensity, ratio, intensity );
                }
        }

        return out;
}