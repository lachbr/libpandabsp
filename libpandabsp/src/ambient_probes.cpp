#include "ambient_probes.h"
#include "bsploader.h"
#include "bspfile.h"
#include "mathlib.h"

#include <shader.h>

#define CHANGED_EPSILON 0.1

AmbientProbeManager::AmbientProbeManager() :
        _loader( nullptr )
{
}

AmbientProbeManager::AmbientProbeManager( BSPLoader *loader ) :
        _loader( loader )
{
}

void AmbientProbeManager::process_ambient_probes()
{
        for ( size_t i = 0; i < g_leafambientindex.size(); i++ )
        {
                dleafambientindex_t *ambidx = &g_leafambientindex[i];
                _probes[i] = pvector<ambientprobe_t>();
                for ( int j = 0; j < ambidx->num_ambient_samples; j++ )
                {
                        dleafambientlighting_t *light = &g_leafambientlighting[ambidx->first_ambient_sample + j];
                        ambientprobe_t probe;
                        probe.leaf = i;
                        probe.fixed8 = LPoint3( light->x, light->y, light->z );
                        probe.cube = PTA_LVecBase3::empty_array( 6 );
                        for ( int k = 0; k < 6; k++ )
                        {
                                probe.cube.set_element( k, color_shift_pixel( &light->cube.color[k], _loader->_gamma ) );
                        }
                        _probes[i].push_back( probe );
                }
        }
}

void AmbientProbeManager::add_node( const NodePath &node )
{
        WeakNodePath *wnp = new WeakNodePath( node );
        _objects.push_back( wnp );
        _object_pos_cache[wnp] = LPoint3( 0 );

        PT( Shader ) shader = Shader::load( Shader::SL_GLSL, Filename( "phase_14/models/shaders/bsp_dynamic_v.glsl" ),
                                            Filename( "phase_14/models/shaders/bsp_dynamic_f.glsl" ) );
        wnp->get_node_path().set_shader( shader );
        wnp->get_node_path().set_shader_input( "ambient_cube", PTA_LVecBase3::empty_array( 6 ) );
}

INLINE ambientprobe_t *AmbientProbeManager::find_closest_sample( int leaf_id, const LPoint3 &fixed8 )
{
        pvector<ambientprobe_t> &probes = _probes[leaf_id];
        pvector<ambientprobe_t *> samples;
        samples.reserve( probes.size() );
        for ( int i = 0; i < probes.size(); i++ )
        {
                samples[i] = &probes[i];
        }

        std::sort( samples.begin(), samples.end(), [fixed8]( const ambientprobe_t *a, const ambientprobe_t *b )
        {
                return ( a->fixed8 - fixed8 ).length() < ( b->fixed8 - fixed8 ).length();
        } );

        return samples[0];
}

void AmbientProbeManager::update()
{
        for ( size_t i = 0; i < _objects.size(); i++ )
        {
                WeakNodePath *wnp = _objects[i];
                if ( wnp->was_deleted() )
                {
                        _object_pos_cache.erase( wnp );
                        _objects.erase( std::find( _objects.begin(), _objects.end(), wnp ) );
                        delete wnp;
                        i--;
                        continue;
                }

                NodePath np = wnp->get_node_path();
                LPoint3 &last_pos = _object_pos_cache[wnp];
                LPoint3 curr_pos = np.get_pos( _loader->_result );
                if ( ( curr_pos - last_pos ).length() >= CHANGED_EPSILON )
                {
                        int leaf_id = _loader->find_leaf( curr_pos );
                        if ( _probes[leaf_id].size() == 0 )
                        {
                                continue;
                        }

                        dleaf_t *leaf = g_dleafs + leaf_id;
                        // Get a fixed 8 for the position in the leaf.
                        LPoint3 fixed8( fixed_8_fraction( curr_pos[0], leaf->mins[0], leaf->maxs[0] ),
                                        fixed_8_fraction( curr_pos[1], leaf->mins[1], leaf->maxs[1] ),
                                        fixed_8_fraction( curr_pos[2], leaf->mins[2], leaf->maxs[2] ) );
                        
                        ambientprobe_t *sample = find_closest_sample( leaf_id, fixed8 );
                        np.set_shader_input( "ambient_cube", sample->cube );
                }
        }
}