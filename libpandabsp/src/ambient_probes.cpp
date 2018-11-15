/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file ambient_probes.cpp
 * @author Brian Lach
 * @date August 03, 2018
 */

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
#include <material.h>
#include <auxBitplaneAttrib.h>
#include <alphaTestAttrib.h>
#include <colorBlendAttrib.h>
#include <materialAttrib.h>
#include <clipPlaneAttrib.h>
#include <virtualFileSystem.h>
#include <modelNode.h>
#include <pstatTimer.h>
#include <lineSegs.h>

#include <bitset>

#include "bsp_trace.h"

static PStatCollector findsky_collector( "AmbientProbes:FindLight/Sky" );
static PStatCollector updatenode_collector              ( "AmbientProbes:UpdateNodes" );
static PStatCollector finddata_collector                ( "AmbientProbes:UpdateNodes:FindNodeData" );
static PStatCollector update_ac_collector               ( "AmbientProbes:UpdateNodes:UpdateAmbientCube" );
static PStatCollector update_locallights_collector      ( "AmbientProbes:UpdateNodes:UpdateLocalLights" );
static PStatCollector interp_ac_collector               ( "AmbientProbes:UpdateNodes:InterpAmbientCube" );
static PStatCollector copystate_collector               ( "AmbientProbes:UpdateNodes:CopyState" );
static PStatCollector addlights_collector               ( "AmbientProbes:UpdateNodes:AddLights" );
static PStatCollector fadelights_collector              ( "AmbientProbes:UpdateNodes:FadeLights" );
static PStatCollector xformlight_collector( "AmbientProbes:XformLight" );

static LineSegs dbg_trace_segs;
static NodePath dbg_trace_np( "dbg_trace" );

static ConfigVariableBool cfg_lightaverage
( "light-average", true, "Activates/deactivate light averaging" );

static ConfigVariableDouble cfg_lightinterp
( "light-lerp-speed", 5.0, "Controls the speed of light interpolation, 0 turns off interpolation" );

using std::cos;
using std::sin;

#define CHANGED_EPSILON 0.1
#define GARBAGECOLLECT_TIME 10.0
//#define VISUALIZE_AMBPROBES

static light_t *dummy_light = new light_t;

AmbientProbeManager::AmbientProbeManager() :
        _loader( nullptr ),
        _sunlight( nullptr )
{
        dummy_light->id = -1;
        dummy_light->leaf = 0;
        dummy_light->type = 0;
}

AmbientProbeManager::AmbientProbeManager( BSPLoader *loader ) :
        _loader( loader ),
        _sunlight( nullptr )
{
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
                        light->id = entnum;

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
                                if ( light->type == LIGHTTYPE_SUN )
                                {
                                        // Flip light direction to match Panda DirectionalLights.
                                        light->direction = -light->direction;
                                }
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

        //findsky_collector.start();

        LPoint3 start( ( point + LPoint3( 0, 0, 0.05 ) ) * 16 );
        LPoint3 end = start + ( _sunlight->direction.get_xyz() * 10000 );
        Ray ray( start, end, LPoint3::zero(), LPoint3::zero() );
        FaceFinder finder( _loader->_bspdata );
        bool ret = !finder.find_intersection( ray );
        //Trace trace;
        //CM_BoxTrace( ray, 0, CONTENTS_SOLID, false, _loader->_bspdata, trace );

        //findsky_collector.stop();

        //return !trace.has_hit();
        return ret;
}

INLINE bool AmbientProbeManager::is_light_visible( const LPoint3 &point, const light_t *light )
{
        //findsky_collector.start();

        Ray ray( ( point + LPoint3( 0, 0, 0.05 ) ) * 16, light->pos * 16, LPoint3::zero(), LPoint3::zero() );
        FaceFinder finder( _loader->_bspdata );
        bool ret = !finder.find_intersection( ray );
        //Trace trace;
        //CM_BoxTrace( ray, 0, CONTENTS_SOLID, false, _loader->_bspdata, trace );

        //if ( !dbg_trace_np.is_empty() )
        //        dbg_trace_np.remove_node();
        //dbg_trace_segs.set_color( LColor( 1.0, 0.0, 0.0, 1.0 ) );
        //dbg_trace_segs.move_to( ray.start / 16 );
        //dbg_trace_segs.draw_to( ray.end / 16 );
        

        //findsky_collector.stop();

        //return !trace.has_hit();
        return ret;
}

INLINE void AmbientProbeManager::garbage_collect_cache()
{
        for ( size_t i = 0; i < _pos_cache.size(); )
        {
                if ( _pos_cache.get_key( i ).was_deleted() )
                {
                        _pos_cache.remove_element( i++ );
                }
                else
                {
                        i++;
                }
        }

        for ( size_t i = 0; i < _node_data.size(); )
        {
                if ( _node_data.get_key( i ).was_deleted() )
                {
                        _node_data.remove_element( i++ );
                }
                else
                {
                        i++;
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

PT( nodeshaderinput_t ) AmbientProbeManager::update_node( PandaNode *node,
                                                          const TransformState *curr_trans )
{
        PStatTimer timer( updatenode_collector );

        if ( !node || !curr_trans )
        {
                return nullptr;
        }

        finddata_collector.start();
        bool new_instance = false;
        if ( _node_data.find( node ) == -1 )
        {
                // This is a new node we have encountered.
                new_instance = true;
        }

        PT( nodeshaderinput_t ) input;
        if ( new_instance )
        {
                input = new nodeshaderinput_t;
                input->node_sequence = _node_sequence++;
                _node_data[node] = input;
                _pos_cache[node] = curr_trans;
        }
        else
        {
                input = _node_data[node];
        }

        finddata_collector.stop();

        if ( input == nullptr )
        {
                return nullptr;
        }

        // Is it even necessary to update anything?
        CPT( TransformState ) prev_trans = _pos_cache[node];
        LVector3 pos_delta( 0 );
        if ( prev_trans != nullptr )
        {
                pos_delta = curr_trans->get_pos() - prev_trans->get_pos();
        }
        else
        {
                _pos_cache[node] = curr_trans;
        }

        bool pos_changed = pos_delta.length() >= EQUAL_EPSILON || new_instance;

        double now = ClockObject::get_global_clock()->get_frame_time();
        float dt = now - input->lighting_time;
        if ( dt <= 0.0 )
        {
                dt = 0.0;
        }
        else
        {
                input->lighting_time = now;
        }

        float atten_factor = exp( -cfg_lightinterp.get_value() * dt );

        LPoint3 curr_net = curr_trans->get_pos();
        int leaf_id = _loader->find_leaf( curr_net );

        update_ac_collector.start();
        if ( pos_changed )
        {
                // Update ambient cube
                if ( _probes[leaf_id].size() > 0 )
                {
                        ambientprobe_t *sample = find_closest_sample( leaf_id, curr_net );
                        input->amb_probe = sample;

#ifdef VISUALIZE_AMBPROBES
                        std::cout << "Box colors:" << std::endl;
                        for ( int i = 0; i < 6; i++ )
                        {
                                std::cout << "\t" << sample->cube[i] << std::endl;
                        }
                        for ( size_t j = 0; j < _probes[leaf_id].size(); j++ )
                        {
                                _probes[leaf_id][j].visnode.set_color_scale( LColor( 0, 0, 1, 1 ), 1 );
                        }
                        if ( !sample->visnode.is_empty() )
                        {
                                sample->visnode.set_color_scale( LColor( 0, 1, 0, 1 ), 1 );
                        }
#endif
                }

                // Cache the last position.
                _pos_cache[node] = curr_trans;
        }
        update_ac_collector.stop();

        interp_ac_collector.start();
        if ( input->amb_probe )
        {
                // Interpolate ambient probe colors
                LVector3 delta( 0 );
                for ( int i = 0; i < 6; i++ )
                {
                        if ( cfg_lightaverage.get_value() && input->amb_probe->cube[i] != input->ambient_cube[i] )
                        {
                                delta = input->amb_probe->cube[i] - input->ambient_cube[i];
                                delta *= atten_factor;
                        }
                        input->ambient_cube[i] = input->amb_probe->cube[i] - delta;
                }
        }
        interp_ac_collector.stop();

        update_locallights_collector.start();
        if ( pos_changed )
        {
                input->occluded_lights.reset();

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

                input->locallights = locallights;
                input->sky_idx = sky_idx;
        }
        update_locallights_collector.stop();
        
        size_t numlights = input->locallights.size();
        int lights_updated = 0;

        int match[MAX_TOTAL_LIGHTS];
        memset( match, 0, sizeof( int ) * MAX_TOTAL_LIGHTS );

        copystate_collector.start();
        nodeshaderinput_t oldstate = nodeshaderinput_t( *input );
        oldstate.local_object();
        copystate_collector.stop();

        input->active_lights = 0;

        addlights_collector.start();
        // Add lights for new state
        for ( size_t i = 0; i < numlights && lights_updated < MAX_ACTIVE_LIGHTS; i++ )
        {
                light_t *light = input->locallights[i];

                bool occluded = false;

                if ( pos_changed )
                {
                        if ( i != input->sky_idx && !is_light_visible( curr_net, light ) )
                        {
                                // The light is occluded, don't add it.
                                input->occluded_lights.set( i );
                                occluded = true;
                        }
                }

                if ( occluded || input->occluded_lights.test( i ) )
                {
                        // light occluded
                        continue;
                }

                input->light_type[lights_updated] = light->type;

                // Pack the light data into a 4x4 matrix.
                LMatrix4f data;
                data.set_row( 0, light->eye_pos );
                data.set_row( 1, light->eye_direction );
                data.set_row( 2, light->atten );
                data.set_row( 3, light->color );

                LVector3 interp_light( 0 );

                // Check for the same light in the old state to interpolate the color.
                for ( size_t j = 0; j < oldstate.light_count[0]; j++ )
                {
                        if ( oldstate.light_ids[j] == light->id )
                        {
                                // This light also existed in the old state.
                                // Interpolate the color.
                                LVector3 oldcolor = LMatrix4( oldstate.light_data[j] ).get_row3( 3 );
                                if ( j < oldstate.active_lights )
                                {
                                        // Only record a match if this light was
                                        // also active in the old state.
                                        match[j] = 1;
                                }
                                interp_light = oldcolor;
                                break;
                        }
                }

                LVector3 delta = light->color - interp_light;
                delta *= atten_factor;
                data.set_row( 3, light->color - delta );

                input->light_data[lights_updated] = data;
                input->light_ids[lights_updated] = light->id;
                input->active_lights++;

                lights_updated++;
        }
        addlights_collector.stop();

        fadelights_collector.start();
        // Fade out any lights that were removed in the new state
        for ( size_t i = 0; i < oldstate.light_count[0]; i++ )
        {
                if ( match[i] == 1 )
                {
                        continue;
                }
                LVector3 color = LMatrix4( oldstate.light_data[i] ).get_row3( 3 );
                if ( color.length_squared() < 1.0 )
                {
                        continue;
                }

                if ( lights_updated >= MAX_TOTAL_LIGHTS )
                {
                        break;
                }

                LVector3 new_color = color * atten_factor;

                input->light_type[lights_updated] = oldstate.light_type[i];
                // Pack the light data into a 4x4 matrix.
                LMatrix4f data = oldstate.light_data[i];
                data.set_row( 3, new_color );
                input->light_data[lights_updated] = data;
                input->light_ids[lights_updated] = oldstate.light_ids[i];

                lights_updated++;
        }
        fadelights_collector.stop();

        input->light_count[0] = lights_updated;

        return input;
}

INLINE void xform_light( light_t *light, const LMatrix4 &cam_mat )
{
        if ( light->type != LIGHTTYPE_SUN ) // sun has no position, just direction
        {
                light->eye_pos = cam_mat.xform( LVector4( light->pos, 1.0 ) );
        }

        if ( light->type != LIGHTTYPE_POINT ) // point lights have no directional component
        {
                light->eye_direction = cam_mat.xform( light->direction );
        }
}

/**
 * Transforms all potentially visible lights into eye space.
 */
void AmbientProbeManager::xform_lights( const TransformState *cam_trans )
{
        if ( dbg_trace_np.is_empty() )
        {
                dbg_trace_np = _loader->_render.attach_new_node( dbg_trace_segs.create() );
                dbg_trace_np.set_shader_off();
                dbg_trace_segs.reset();
        }
        
        

        PStatTimer timer( xformlight_collector );

        LMatrix4 cam_mat = cam_trans->get_mat();

        int cam_leaf = _loader->find_leaf( cam_trans->get_pos() );
        const pvector<light_t *> &lights = _light_pvs[cam_leaf];
        size_t numlights = lights.size();
        for ( size_t i = 0; i < numlights; i++ )
        {
                light_t *light = lights[i];
                xform_light( light, cam_mat );
        }

        if ( _sunlight != nullptr )
        {
                xform_light( _sunlight, cam_mat );
        }
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

void AmbientProbeManager::cleanup()
{
        _sunlight = nullptr;
        _pos_cache.clear();
        _node_data.clear();
        _probes.clear();
        _all_probes.clear();
        _light_pvs.clear();
        _all_lights.clear();
}