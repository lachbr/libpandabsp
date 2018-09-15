#include "ambient_probes.h"
#include "bsploader.h"
#include "bspfile.h"
#include "mathlib.h"
#include "bsptools.h"
#include "winding.h"

#include <shader.h>
#include <loader.h>
#include <textNode.h>
#include <pStatCollector.h>
#include <clockObject.h>

#include "bsp_trace.h"

static PStatCollector findsky_collector( "AmbientProbes:FindLight/Sky" );
static PStatCollector updatenode_collector( "AmbientProbes:UpdateNodes" );

using std::cos;
using std::sin;

#define CHANGED_EPSILON 0.1
#define GARBAGECOLLECT_TIME 10.0
//#define VISUALIZE_AMBPROBES

CPT( ShaderAttrib ) AmbientProbeManager::_identity_shattr = nullptr;
CPT( Shader ) AmbientProbeManager::_shader = nullptr;

AmbientProbeManager::AmbientProbeManager() :
        _loader( nullptr ),
        _sunlight( nullptr )
{
}

AmbientProbeManager::AmbientProbeManager( BSPLoader *loader ) :
        _loader( loader ),
        _sunlight( nullptr )
{
}

CPT( Shader ) AmbientProbeManager::get_shader()
{
        if ( _shader == nullptr )
        {
                _shader = Shader::load( Shader::SL_GLSL, Filename( "phase_14/models/shaders/bsp_dynamic_v.glsl" ),
                                        Filename( "phase_14/models/shaders/bsp_dynamic_f.glsl" ) );
        }

        return _shader;
}

CPT( ShaderAttrib ) AmbientProbeManager::get_identity_shattr()
{
        if ( _identity_shattr == nullptr )
        {
                CPT( ShaderAttrib ) shattr = DCAST( ShaderAttrib, ShaderAttrib::make( get_shader() ) );
                shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "ambient_cube", PTA_LVecBase3::empty_array( 6 ) ) ) );
                shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "num_locallights", PTA_int::empty_array( 1 ) ) ) );
                shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "locallight_type", PTA_int::empty_array( 2 ) ) ) );
                for ( int i = 0; i < 2; i++ )
                {
                        std::stringstream ss;
                        ss << "locallight[" << i << "]";
                        std::string name = ss.str();
                        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( name + ".pos", LVector3( 0 ) ) ) );
                        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( name + ".direction", LVector4( 0 ) ) ) );
                        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( name + ".atten", LVector4( 0 ) ) ) );
                        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( name + ".color", LVector3( 1 ) ) ) );
                }

                _identity_shattr = shattr;
        }

        return _identity_shattr;
}

INLINE LVector3 angles_to_vector( const vec3_t &angles )
{
        return LVector3( cos( deg_2_rad(angles[0]) ) * cos( deg_2_rad(angles[1]) ),
                         sin( deg_2_rad(angles[0]) ) * cos( deg_2_rad(angles[1]) ),
                         sin( deg_2_rad(angles[1]) ) );
}

INLINE int lighttype_from_classname( const char *classname )
{
        if ( !strncmp( classname, "light_environment", 18 ) )
        {
                return LIGHTTYPE_SUN;
        }
        else if ( !strncmp( classname, "light_spot", 11 ) )
        {
                return LIGHTTYPE_SPOT;
        }
        else if ( !strncmp( classname, "light", 5 ) )
        {
                return LIGHTTYPE_POINT;
        }

        return LIGHTTYPE_POINT;
}

void AmbientProbeManager::process_ambient_probes()
{

#ifdef VISUALIZE_AMBPROBES
        if ( !_vis_root.is_empty() )
        {
                _vis_root.remove_node();
        }
        _vis_root = _loader->_result.attach_new_node( "visAmbProbesRoot" );
#endif

        _light_pvs.clear();
        _light_pvs.resize( _loader->_bspdata->dmodels[0].visleafs + 1 );
        _all_lights.clear();

        // Build light data structures
        for ( int entnum = 0; entnum < _loader->_bspdata->numentities; entnum++ )
        {
                entity_t *ent = _loader->_bspdata->entities + entnum;
                const char *classname = ValueForKey( ent, "classname" );
                if ( !strncmp( classname, "light", 5 ) )
                {
                        PT( light_t ) light = new light_t;

                        vec3_t pos;
                        GetVectorForKey( ent, "origin", pos );
                        VectorCopy( pos, light->pos );
                        VectorScale( light->pos, 1 / 16.0, light->pos );

                        light->leaf = _loader->find_leaf( light->pos );
                        light->color = color_from_value( ValueForKey( ent, "_light" ) ).get_xyz();
                        light->type = lighttype_from_classname( classname );

                        if ( light->type == LIGHTTYPE_SUN ||
                             light->type == LIGHTTYPE_SPOT)
                        {
                                vec3_t angles;
                                GetVectorForKey( ent, "angles", angles );
                                float pitch = FloatForKey( ent, "pitch" );
                                float temp = angles[1];
                                if ( !pitch )
                                {
                                        pitch = angles[0];
                                }
                                angles[1] = pitch;
                                angles[0] = temp;
                                VectorCopy( angles_to_vector( angles ), light->direction );
                                light->direction[3] = 0.0;
                        }

                        if ( light->type == LIGHTTYPE_SPOT ||
                             light->type == LIGHTTYPE_POINT )
                        {
                                float fade = FloatForKey( ent, "_fade" );
                                light->atten = LVector4( fade / 16.0, 0, 0, 0 );
                                if ( light->type == LIGHTTYPE_SPOT )
                                {
                                        float stopdot = FloatForKey( ent, "_cone" );
                                        if ( !stopdot )
                                        {
                                                stopdot = 10;
                                        }
                                        float stopdot2 = FloatForKey( ent, "_cone2" );
                                        if ( !stopdot2 )
                                        {
                                                stopdot2 = stopdot;
                                        }
                                        if ( stopdot2 < stopdot )
                                        {
                                                stopdot2 = stopdot;
                                        }

                                        stopdot = (float)std::cos( stopdot / 180 * Q_PI );
                                        stopdot2 = (float)std::cos( stopdot2 / 180 * Q_PI );

                                        light->atten[1] = stopdot;
                                        light->atten[2] = stopdot2;
                                }
                        }

                        _all_lights.push_back( light );
                        if ( light->type == LIGHTTYPE_SUN )
                        {
                                _sunlight = light;
                        }
                }
        }

        // Build light PVS
        for ( size_t lightnum = 0; lightnum < _all_lights.size(); lightnum++ )
        {
                light_t *light = _all_lights[lightnum];
                for ( int leafnum = 0; leafnum < _loader->_bspdata->dmodels[0].visleafs + 1; leafnum++ )
                {
                        if ( light->type != LIGHTTYPE_SUN &&
                             _loader->is_cluster_visible(light->leaf, leafnum) )
                        {
                                _light_pvs[leafnum].push_back( light );
                        }
                }
        }

        for ( size_t i = 0; i < _loader->_bspdata->leafambientindex.size(); i++ )
        {
                dleafambientindex_t *ambidx = &_loader->_bspdata->leafambientindex[i];
                dleaf_t *leaf = _loader->_bspdata->dleafs + i;
                _probes[i] = pvector<ambientprobe_t>();
                for ( int j = 0; j < ambidx->num_ambient_samples; j++ )
                {
                        dleafambientlighting_t *light = &_loader->_bspdata->leafambientlighting[ambidx->first_ambient_sample + j];
                        ambientprobe_t probe;
                        probe.leaf = i;
                        probe.pos = LPoint3( RemapValClamped( light->x, 0, 255, leaf->mins[0], leaf->maxs[0] ),
                                             RemapValClamped( light->y, 0, 255, leaf->mins[1], leaf->maxs[1] ),
                                             RemapValClamped( light->z, 0, 255, leaf->mins[2], leaf->maxs[2] ) );
                        probe.pos /= 16.0;
                        probe.cube = PTA_LVecBase3::empty_array( 6 );
                        for ( int k = 0; k < 6; k++ )
                        {
                                probe.cube.set_element( k, color_shift_pixel( &light->cube.color[k], _loader->_gamma ) );
                        }
#ifdef VISUALIZE_AMBPROBES
                        PT( TextNode ) tn = new TextNode( "visText" );
                        char text[40];
                        sprintf( text, "Ambient Sample %i\nLeaf %i", j, i );
                        tn->set_align( TextNode::A_center );
                        tn->set_text( text );
                        tn->set_text_color( LColor( 1, 1, 1, 1 ) );
                        NodePath smiley = _vis_root.attach_new_node( tn->generate() );
                        smiley.set_pos( probe.pos );
                        smiley.set_billboard_axis();
                        smiley.clear_model_nodes();
                        smiley.flatten_strong();
                        probe.visnode = smiley;
#endif
                        _probes[i].push_back( probe );
                }
        }
}

INLINE bool AmbientProbeManager::is_sky_visible( const LPoint3 &point )
{
        if ( _sunlight == nullptr )
        {
                return false;
        }

        findsky_collector.start();

        LPoint3 start( ( point + LPoint3( 0, 0, 0.05 ) ) * 16 );
        LPoint3 end = start - ( _sunlight->direction.get_xyz() * 10000 );
        Ray ray( start, end, LPoint3::zero(), LPoint3::zero() );
        FaceFinder finder( _loader->_bspdata );
        bool ret = !finder.find_intersection( ray );

        findsky_collector.stop();

        return ret;
}

INLINE bool AmbientProbeManager::is_light_visible( const LPoint3 &point, const light_t *light )
{
        findsky_collector.start();

        Ray ray( ( point + LPoint3( 0, 0, 0.05 ) ) * 16, light->pos * 16, LPoint3::zero(), LPoint3::zero() );
        FaceFinder finder( _loader->_bspdata );
        bool ret = !finder.find_intersection( ray );

        findsky_collector.stop();

        return ret;
}

INLINE void AmbientProbeManager::garbage_collect_cache()
{
        for ( auto itr = _pos_cache.cbegin(); itr != _pos_cache.cend(); )
        {
                if ( itr->first.was_deleted() )
                {
                        _pos_cache.erase( itr++ );
                }
                else
                {
                        itr++;
                }
        }
}

void AmbientProbeManager::consider_garbage_collect()
{
        double now = ClockObject::get_global_clock()->get_frame_time();
        if ( now - _last_garbage_collect_time >= GARBAGECOLLECT_TIME )
        {
                garbage_collect_cache();
                _last_garbage_collect_time = now;
        }
}

void AmbientProbeManager::update_node( PandaNode *node, CPT( TransformState ) curr_trans )
{
        if ( !node || !curr_trans )
        {
                return;
        }
        
        updatenode_collector.start();

        CPT( ShaderAttrib ) shattr = nullptr;
        bool new_instance = false;
        if ( !node->has_attrib( ShaderAttrib::get_class_type() ) )
        {
                shattr = get_identity_shattr();
                new_instance = true;
        }
        else
        {
                shattr = DCAST( ShaderAttrib, node->get_attrib( ShaderAttrib::get_class_type() ) );
        }

        // Is it even necessary to update anything?
        if ( new_instance )
        {
                _pos_cache[node] = curr_trans;
        }
        CPT( TransformState ) prev_trans = _pos_cache[node];
        LVector3 pos_delta( 0 );
        if ( prev_trans != nullptr )
        {
                pos_delta = curr_trans->get_pos() - prev_trans->get_pos();
        }
        if ( pos_delta.length() >= CHANGED_EPSILON || new_instance )
        {
                LPoint3 curr_net = curr_trans->get_pos();
                int leaf_id = _loader->find_leaf( curr_net );

                // Update ambient cube
                if ( _probes[leaf_id].size() > 0 )
                {
                        ambientprobe_t *sample = find_closest_sample( leaf_id, curr_net );

#ifdef VISUALIZE_AMBPROBES
                        for ( size_t j = 0; j < _probes[leaf_id].size(); j++ )
                        {
                                _probes[leaf_id][j].visnode.set_color_scale( LColor( 0, 0, 1, 1 ), 1 );
                        }
                        if ( !sample->visnode.is_empty() )
                        {
                                sample->visnode.set_color_scale( LColor( 0, 1, 0, 1 ), 1 );
                        }
#endif
                        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "ambient_cube", sample->cube ) ) );
                }

                // Update local light sources
                pvector<light_t *> locallights = _light_pvs[leaf_id];
                // Sort local lights from closest to furthest distance from node, we will choose the two closest lights.
                std::sort( locallights.begin(), locallights.end(), [curr_net]( const light_t *a, const light_t *b )
                {
                        return ( a->pos - curr_net ).length() < ( b->pos - curr_net ).length();
                } );

                int sky_idx = -1;
                if ( is_sky_visible( curr_net ) )
                {
                        // If we hit the sky from current position, sunlight takes
                        // precedence over all other local light sources.
                        locallights.insert( locallights.begin(), _sunlight );
                        sky_idx = 0;
                }

                size_t numlights = locallights.size();
                PTA_int lighttypes = PTA_int::empty_array( 2 );
                int lights_updated = 0;
                for ( size_t i = 0; i < numlights && lights_updated < 2; i++ )
                {
                        light_t *light = locallights[i];
                        if ( i != sky_idx && !is_light_visible( curr_net, light ) )
                        {
                                // The light is occluded, don't add it.
                                continue;
                        }
                        lighttypes.set_element( lights_updated, light->type );
                        stringstream ss;
                        ss << "locallight[" << lights_updated << "]";
                        std::string lightname = ss.str();
                        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( lightname + ".pos", light->pos ) ) );
                        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( lightname + ".color", light->color ) ) );
                        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( lightname + ".direction", light->direction ) ) );
                        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( lightname + ".atten", light->atten ) ) );
                        lights_updated++;
                }
                PTA_int pta_numlights = PTA_int::empty_array( 1 );
                pta_numlights.set_element( 0, lights_updated );
                shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "num_locallights", pta_numlights ) ) );
                shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "locallight_type", lighttypes ) ) );

                // Apply the modified shader attrib.
                node->set_attrib( shattr );

                // Cache the last position.
                _pos_cache[node] = curr_trans;
        }

        updatenode_collector.stop();
}

INLINE ambientprobe_t *AmbientProbeManager::find_closest_sample( int leaf_id, const LPoint3 &pos )
{
        pvector<ambientprobe_t> &probes = _probes[leaf_id];
        pvector<ambientprobe_t *> samples;
        samples.resize( probes.size() );
        for ( int i = 0; i < probes.size(); i++ )
        {
                samples[i] = &probes[i];
        }

        std::sort( samples.begin(), samples.end(), [pos]( const ambientprobe_t *a, const ambientprobe_t *b )
        {
                return ( a->pos - pos ).length() < ( b->pos - pos ).length();
        } );

        return samples[0];
}