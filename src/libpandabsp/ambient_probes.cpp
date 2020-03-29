/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
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
#include "ambient_boost_effect.h"
#include "lighting_origin_effect.h"

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
#include "aux_data_attrib.h"

TypeDef( nodeshaderinput_t );

static PStatCollector updatenode_collector              ( "AmbientProbes:UpdateNodes" );
static PStatCollector finddata_collector                ( "AmbientProbes:UpdateNodes:FindNodeData" );
static PStatCollector update_ac_collector               ( "AmbientProbes:UpdateNodes:UpdateAmbientCube" );
static PStatCollector update_locallights_collector      ( "AmbientProbes:UpdateNodes:UpdateLocalLights" );
static PStatCollector interp_ac_collector               ( "AmbientProbes:UpdateNodes:InterpAmbientCube" );
static PStatCollector copystate_collector               ( "AmbientProbes:UpdateNodes:CopyState" );
static PStatCollector addlights_collector               ( "AmbientProbes:UpdateNodes:AddLights" );
static PStatCollector fadelights_collector              ( "AmbientProbes:UpdateNodes:FadeLights" );
static PStatCollector ambientboost_collector( "AmbientProbes:UpdateNodes:BoostAmbient" );
static PStatCollector xformlight_collector( "AmbientProbes:XformLight" );
static PStatCollector loadcubemap_collector( "AmbientProbes:UpdateNodes:LoadCubemap" );
static PStatCollector findcubemap_collector( "AmbientProbes:UpdateNodes:FindCubemap" );

static ConfigVariableBool cfg_lightaverage
( "light-average", true, "Activates/deactivate light averaging" );
static ConfigVariableDouble cfg_lightinterp
( "light-lerp-speed", 5.0, "Controls the speed of light interpolation, 0 turns off interpolation" );

static ConfigVariableBool r_ambientboost
( "r_ambientboost", true, "Boosts ambient term if it is totally swamped by local lights." );
static ConfigVariableDouble r_ambientmin
( "r_ambientmin", 0.3, "Threshold above which ambient cube will not boost (i.e. it's already sufficiently bright)." );
static ConfigVariableDouble r_ambientfraction
( "r_ambientfraction", 0.1, "Cheat. Fraction of direct lighting that ambient cube must be below to trigger boosting." );
static ConfigVariableDouble r_ambientfactor
( "r_ambientfactor", 5.0, "Boost ambient cube by no more than this factor." );

using std::cos;
using std::sin;

#define CHANGED_EPSILON 0.1
#define GARBAGECOLLECT_TIME 10.0
//#define VISUALIZE_AMBPROBES

static light_t *dummy_light = new light_t;

struct nodecallbackdata_t
{
        PandaNode *node;
        WeakReferenceList *list;
        AmbientProbeManager *mgr;
};

class NodeWeakCallback : public WeakPointerCallback
{
PUBLISHED:
        virtual void wp_callback( void *pointer );
};

void NodeWeakCallback::wp_callback( void *pointer )
{
        nodecallbackdata_t *data = (nodecallbackdata_t *)pointer;

        MutexHolder holder( data->mgr->_cache_mutex );

        int itr = data->mgr->_node_data.find( data->node );
        if ( itr != -1 )
        {
                data->mgr->_node_data.remove_element( itr );
        }

        itr = data->mgr->_pos_cache.find( data->node );
        if ( itr != -1 )
        {
                data->mgr->_pos_cache.remove_element( itr );
        }

        delete data;
        delete this;
}

AmbientProbeManager::AmbientProbeManager() :
        _loader( nullptr ),
        _sunlight( nullptr ),
        //_light_kdtree( nullptr ),
        //_probe_kdtree( nullptr ),
        _envmap_kdtree( nullptr )
{
        dummy_light->id = -1;
        dummy_light->leaf = 0;
        dummy_light->type = 0;
}

AmbientProbeManager::AmbientProbeManager( BSPLoader *loader ) :
        _loader( loader ),
        _sunlight( nullptr ),
        //_light_kdtree( nullptr ),
        //_probe_kdtree( nullptr ),
        _envmap_kdtree( nullptr )
{
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

        //_light_kdtree = new KDTree( 3 );
        //vector<vector<double>> light_points;

        // Build light data structures
        for ( int entnum = 0; entnum < _loader->_bspdata->numentities; entnum++ )
        {
                entity_t *ent = _loader->_bspdata->entities + entnum;
                const char *classname = ValueForKey( ent, "classname" );
                if ( !strncmp( classname, "light", 5 ) )
                {
                        PT( light_t ) light = new light_t;
                        light->id = (int)_all_lights.size();

                        vec3_t pos;
                        GetVectorDForKey( ent, "origin", pos );
                        VectorCopy( pos, light->pos );
                        VectorScale( light->pos, 1 / 16.0, light->pos );

                        light->leaf = _loader->find_leaf( light->pos );
                        light->color = color_from_value( ValueForKey( ent, "_light" ), true, true ).get_xyz();
                        light->type = lighttype_from_classname( classname );

                        lightfalloffparams_t params = GetLightFalloffParams( ent, light->color );

                        if ( light->type == LIGHTTYPE_SUN ||
                             light->type == LIGHTTYPE_SPOT)
                        {
                                vec3_t angles;
                                GetVectorDForKey( ent, "angles", angles );
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
                                light->falloff = LVector4( params.constant_atten, params.linear_atten, params.quadratic_atten, 0 );
                                light->falloff2 = LVector4( params.start_fade_distance, params.end_fade_distance, params.cap_distance, 0 );
                                light->falloff3 = LVector4( 0, 0, 0, 0 );
                                if ( light->type == LIGHTTYPE_SPOT )
                                {
                                        float stopdot = FloatForKey( ent, "_inner_cone" );
                                        if ( !stopdot )
                                        {
                                                stopdot = 10;
                                        }
                                        float stopdot2 = FloatForKey( ent, "_cone" );
                                        if ( !stopdot2 || stopdot2 < stopdot )
                                        {
                                                stopdot2 = stopdot;
                                        }
                                        if ( stopdot2 < stopdot )
                                        {
                                                stopdot2 = stopdot;
                                        }

                                        // this is a point light if stop dots are 180
                                        if ( stopdot == 180 && stopdot2 == 180 )
                                        {
                                                light->type = LIGHTTYPE_POINT;
                                        }
                                        else
                                        {
                                                stopdot = (float)std::cos( stopdot / 180 * Q_PI );
                                                stopdot2 = (float)std::cos( stopdot2 / 180 * Q_PI );
                                                float exp = FloatForKey( ent, "_exponent" );

                                                light->falloff3[0] = exp;

                                                light->falloff[3] = stopdot;
                                                light->falloff2[3] = stopdot2;
                                        }
                                }
                        }

                        _all_lights.push_back( light );
                        if ( light->type == LIGHTTYPE_SUN )
                        {
                                // don't put the sun in the k-d tree
                                _sunlight = light;
                        }
                        //else
                        //{
                        //        // add the light into our k-d tree
                        //        light_points.push_back( { light->pos[0], light->pos[1], light->pos[2] } );
                        //}
                }
        }

        //if ( light_points.size() )
        //{
        //        _light_kdtree->build( light_points );
        //}

        // Build light PVS
        for ( size_t lightnum = 0; lightnum < _all_lights.size(); lightnum++ )
        {
                light_t *light = _all_lights[lightnum];
                for ( int leafnum = 0; leafnum < _loader->_bspdata->dmodels[0].visleafs + 1; leafnum++ )
                {
                        if ( light->type != LIGHTTYPE_SUN &&
                             _loader->is_cluster_visible( light->leaf, leafnum ) )
                        {
                                _light_pvs[leafnum].push_back( light );
                        }
                }
        }

        for ( size_t i = 0; i < _loader->_bspdata->leafambientindex.size(); i++ )
        {
                dleafambientindex_t *ambidx = &_loader->_bspdata->leafambientindex[i];
                dleaf_t *leaf = _loader->_bspdata->dleafs + i;
                _probes[i] = pvector<PT( ambientprobe_t )>();

                _probe_kdtrees[i] = new KDTree( 3 );
                vector<vector<double>> probe_points;

                for ( int j = 0; j < ambidx->num_ambient_samples; j++ )
                {
                        dleafambientlighting_t *light = &_loader->_bspdata->leafambientlighting[ambidx->first_ambient_sample + j];
                        PT( ambientprobe_t ) probe = new ambientprobe_t;
                        probe->leaf = i;
                        probe->pos = LPoint3( RemapValClamped( light->x, 0, 255, leaf->mins[0], leaf->maxs[0] ),
                                             RemapValClamped( light->y, 0, 255, leaf->mins[1], leaf->maxs[1] ),
                                             RemapValClamped( light->z, 0, 255, leaf->mins[2], leaf->maxs[2] ) );
                        probe->pos /= 16.0;

                        bool match = false;
                        for ( size_t k = 0; k < _probes[i].size(); k++ )
                        {
                                if ( _probes[i][k]->pos == probe->pos )
                                {
                                        match = true;
                                        break;
                                }
                        }
                        if ( match )
                                continue;

                        probe->cube = PTA_LVecBase3::empty_array( 6 );
                        for ( int k = 0; k < 6; k++ )
                        {
                                probe->cube.set_element( k, color_shift_pixel( &light->cube.color[k], _loader->_gamma ) );
                        }
#ifdef VISUALIZE_AMBPROBES
                        PT( TextNode ) tn = new TextNode( "visText" );
                        char text[40];
                        sprintf( text, "Ambient Sample %i\nLeaf %i", j, i );
                        tn->set_align( TextNode::A_center );
                        tn->set_text( text );
                        tn->set_text_color( LColor( 1, 1, 1, 1 ) );
                        NodePath smiley = _vis_root.attach_new_node( tn->generate() );
                        smiley.set_pos( probe->pos );
                        smiley.set_billboard_axis();
                        smiley.clear_model_nodes();
                        smiley.flatten_strong();
                        probe->visnode = smiley;
#endif
                        _probes[i].push_back( probe );

                        // insert probe into the k-d tree so we can find them quickly
                        probe_points.push_back( { probe->pos[0], probe->pos[1], probe->pos[2] } );
                        _all_probes.push_back( probe );
                }

                if ( probe_points.size() )
                {
                        _probe_kdtrees[i]->build( probe_points );
                }
        }

        
}

void AmbientProbeManager::load_cubemaps()
{
        std::cout << _loader->_bspdata->cubemaps.size() << " cubemaps " << std::endl;
        _envmap_kdtree = new KDTree( 3 );
        vector<vector<double>> envmap_points;
        for ( size_t i = 0; i < _loader->_bspdata->cubemaps.size(); i++ )
        {
                dcubemap_t *dcm = &_loader->_bspdata->cubemaps[i];
                PT( cubemap_t ) cm = new cubemap_t;
                cm->pos = LVector3( dcm->pos[0] / 16.0, dcm->pos[1] / 16.0, dcm->pos[2] / 16.0 );

                bool match = false;
                for ( size_t j = 0; j < _cubemaps.size(); j++ )
                {
                        if ( _cubemaps[j]->pos == cm->pos )
                        {
                                match = true;
                                break;
                        }
                }
                if ( match )
                        continue;

                cm->leaf = _loader->find_leaf( cm->pos );
                cm->size = dcm->size;
                cm->has_full_cubemap = true;

                // insert into k-d tree
                envmap_points.push_back( { cm->pos[0], cm->pos[1], cm->pos[2] } );

		// Cubemap is in linear space.
                cm->cubemap_tex = new Texture( "cubemap_tex" );
                cm->cubemap_tex->setup_cube_map( dcm->size, Texture::T_unsigned_short, Texture::F_rgb );
                cm->cubemap_tex->set_wrap_u( SamplerState::WM_clamp );
                cm->cubemap_tex->set_wrap_v( SamplerState::WM_clamp );
                cm->cubemap_tex->set_keep_ram_image( true );

                for ( int j = 0; j < 6; j++ ) // for each side in the cubemap_tex
                {
                        if ( dcm->imgofs[j] == -1 )
                        {
                                std::cout << "\tNo image on side " << j << std::endl;
                                cm->has_full_cubemap = false;
                                continue;
                        }

                        int xel = 0;

                        PNMImage img( dcm->size, dcm->size );
			img.set_color_space( ColorSpace::CS_linear );
			img.set_maxval( USHRT_MAX );
                        img.fill( 0 );

                        for ( int y = 0; y < dcm->size; y++ )
                        {
                                for ( int x = 0; x < dcm->size; x++ )
                                {
                                        colorrgbexp32_t *col = &_loader->_bspdata->cubemapdata[dcm->imgofs[j] + xel];
                                        LVector3 vcol;
                                        ColorRGBExp32ToVector( *col, vcol );
                                        img.set_xel( x, y, vcol );
                                        xel++;
                                }
                        }

                        cm->cubemap_tex->load( img, j, 0 );
                        cm->cubemap_images[j] = img;
                }

		cm->cubemap_tex->write( std::cout, 0 );

                _cubemaps.push_back( cm );
        }

        if ( envmap_points.size() )
        {
                _envmap_kdtree->build( envmap_points );
        }      
}

INLINE bool AmbientProbeManager::is_sky_visible( const LPoint3 &point )
{
        if ( _sunlight == nullptr )
        {
                return false;
        }

        LPoint3 start( ( point + LPoint3( 0, 0, 0.05 ) ) * 16 );
        LPoint3 end = start + ( _sunlight->direction.get_xyz() * 10000 );
        Ray ray( start, end, LPoint3::zero(), LPoint3::zero() );
        Trace trace;
        CM_BoxTrace( ray, 0, CONTENTS_SKY | CONTENTS_SOLID, false, _loader->_colldata, trace );

        return trace.has_hit() && trace.hit_contents == CONTENTS_SKY;
}

INLINE bool AmbientProbeManager::is_light_visible( const LPoint3 &point, const light_t *light )
{
        Ray ray( ( point + LPoint3( 0, 0, 0.05 ) ) * 16, light->pos * 16, LPoint3::zero(), LPoint3::zero() );
        Trace trace;
        CM_BoxTrace( ray, 0, CONTENTS_SOLID, false, _loader->_colldata, trace );

        return !trace.has_hit();
}

INLINE LMatrix4 pack_lightdata( const light_t *light )
{
        return LMatrix4( light->pos, light->direction, light->falloff, light->color );
}

INLINE LMatrix4 pack_lightdata2( const light_t *light )
{
        return LMatrix4( light->falloff2, light->falloff3, 0, 0 );
}

INLINE void apply_lightdatas( int n, const light_t *light, nodeshaderinput_t *input, const LMatrix4 &data, bool active )
{
        input->light_ids[n] = light->id;
        input->light_type[n] = light->type;
        input->light_data[n] = data;
        input->light_data2[n] = pack_lightdata2( light );
        if ( active )
                input->active_lights++;
}

const RenderState *AmbientProbeManager::update_node( PandaNode *node,
						     CPT( TransformState ) curr_trans,
						     bool should_update )
{
        PStatTimer timer( updatenode_collector );

        MutexHolder holder( _cache_mutex );

        if ( !node || !curr_trans )
        {
                return nullptr;
        }

	// By default, the lighting position is the position of the node.
	// An effect can be applied to offset the lighting position.
	if ( node->has_effect( LightingOriginEffect::get_class_type() ) )
	{
		const LightingOriginEffect *effect =
			DCAST( LightingOriginEffect,
			       node->get_effect( LightingOriginEffect::get_class_type() ) );
		LQuaternion quat = curr_trans->get_norm_quat();
		LVector3 world_offset = quat.xform( effect->get_lighting_origin() );
		curr_trans = curr_trans->set_pos( curr_trans->get_pos() + world_offset );
	}

        finddata_collector.start();
        bool new_instance = false;

        int itr = _node_data.find( node );
        if ( itr == -1 )
        {
                // This is a new node we have encountered.
                new_instance = true;
        }

        PT( nodeshaderinput_t ) input = nullptr;
        if ( new_instance )
        {
                input = new nodeshaderinput_t;
                input->node_sequence = _node_sequence++;
                input->state_with_input = RenderState::make( AuxDataAttrib::make( input ) );
                _node_data[node] = input;
                _pos_cache[node] = curr_trans;

                // Add a callback to remove this node from our cache when the reference count
                // reaches zero.
                WeakReferenceList *ref = node->weak_ref();
                nodecallbackdata_t *ncd = new nodecallbackdata_t;
                ncd->mgr = this;
                ncd->node = node;
                ncd->list = ref;
                ref->add_callback( new NodeWeakCallback, ncd );
        }
        else
        {
                input = _node_data.get_data( itr );
        }

        finddata_collector.stop();

        if ( input == nullptr )
        {
                return nullptr;
        }

	if ( !should_update && !new_instance )
	{
		// Just retrieving the current state, no updating.
		return input->state_with_input;
	}

        input->cubemap_changed = false;

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

        bool pos_changed = pos_delta.length_squared() >= EQUAL_EPSILON || new_instance;

        bool average_lighting = cfg_lightaverage.get_value();

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
        // Push the Z up a tiny bit.
        // Initially, the position might be a bit under the ground, making the model
        // be in the incorrect leaf, giving incorrect ambient.
        curr_net[2] += ON_EPSILON;

        int leaf_id = _loader->find_leaf( curr_net );

        if ( pos_changed )
        {
                // Update ambient cube
                if ( _probes[leaf_id].size() > 0 )
                {
                        update_ac_collector.start();
                        ambientprobe_t *sample = find_closest_in_kdtree( get_probe_kdtree( leaf_id ), curr_net, _probes[leaf_id] );
                        input->amb_probe = sample;
                        update_ac_collector.stop();

#ifdef VISUALIZE_AMBPROBES
                        std::cout << "Box colors:" << std::endl;
                        for ( int i = 0; i < 6; i++ )
                        {
                                std::cout << "\t" << sample->cube[i] << std::endl;
                        }
                        for ( size_t j = 0; j < _probes[leaf_id].size(); j++ )
                        {
                                _probes[leaf_id][j]->visnode.set_color_scale( LColor( 0, 0, 1, 1 ), 1 );
                        }
                        if ( !sample->visnode.is_empty() )
                        {
                                sample->visnode.set_color_scale( LColor( 0, 1, 0, 1 ), 1 );
                        }
#endif
                }

                // Update envmap
                if ( _cubemaps.size() > 0 )
                {
                        findcubemap_collector.start();
                        cubemap_t *cm = find_closest_in_kdtree( _envmap_kdtree, curr_net, _cubemaps );
                        findcubemap_collector.stop();
                        if ( cm && cm->has_full_cubemap && cm != input->cubemap )
                        {
                                loadcubemap_collector.start();
                                input->cubemap = cm;
				// Cubemap is in linear space
                                input->cubemap_tex->setup_cube_map( cm->size, Texture::T_unsigned_short, Texture::F_rgb );
                                input->cubemap_tex->set_ram_image( cm->cubemap_tex->get_ram_image(),
                                                                   cm->cubemap_tex->get_ram_image_compression(),
                                                                   cm->cubemap_tex->get_ram_image_size() );
                                input->cubemap_changed = true;
                                loadcubemap_collector.stop();
                        }
                        
                }

                // Cache the last position.
                _pos_cache[node] = curr_trans;
        }

        bool ambientcube_changed = false;

        interp_ac_collector.start();
        if ( input->amb_probe )
        {
                // Interpolate ambient probe colors
                LVector3 delta( 0 );
                for ( int i = 0; i < 6; i++ )
                {
                        delta = input->amb_probe->cube[i] - input->boxcolor[i];
                        if ( average_lighting && delta.length_squared() >= EQUAL_EPSILON )
                        {
                                delta *= atten_factor;
                                ambientcube_changed = true;
                        }
                        input->boxcolor[i] = input->amb_probe->cube[i] - delta;
                }
        }
        interp_ac_collector.stop();

        update_locallights_collector.start();
        if ( pos_changed )
        {
                input->occluded_lights.reset();

                // Update local light sources
                input->locallights = _light_pvs[leaf_id];
                // Sort local lights from closest to furthest distance from node, we will choose the two closest lights.
                std::sort( input->locallights.begin(), input->locallights.end(), [curr_net]( const light_t *a, const light_t *b )
                {
                        return ( a->pos - curr_net ).length_squared() < ( b->pos - curr_net ).length_squared();
                } );

                int sky_idx = -1;
                if ( is_sky_visible( curr_net ) )
                {
                        // If we hit the sky from current position, sunlight takes
                        // precedence over all other local light sources.
			input->locallights.insert( input->locallights.begin(), _sunlight );
                        sky_idx = 0;
                }

                input->sky_idx = sky_idx;
        }
        update_locallights_collector.stop();
        
        size_t numlights = input->locallights.size();
        int lights_updated = 0;

        int match[MAX_TOTAL_LIGHTS];
        memset( match, 0, sizeof( int ) * MAX_TOTAL_LIGHTS );

        int old_lightcount[1];
        int old_lightids[MAX_TOTAL_LIGHTS];
        LMatrix4 old_lightdata[MAX_TOTAL_LIGHTS];
        int old_activelights;

        if ( average_lighting )
        {
                copystate_collector.start();

                // =====================================================================
                // copy all of the old inputs
                
                memcpy( old_lightcount, input->light_count.p(), sizeof( int ) );
                memcpy( old_lightids, input->light_ids.p(), sizeof( int ) * input->light_ids.size() );
                memcpy( old_lightdata, input->light_data.p(), sizeof( LMatrix4 ) * input->light_data.size() );
                old_activelights = input->active_lights;
                
                // =====================================================================

                copystate_collector.stop();
        }

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

                LVector3 interp_light( 0 );

                if ( average_lighting )
                {
                        // Check for the same light in the old state to interpolate the color.
                        for ( size_t j = 0; j < old_lightcount[0]; j++ )
                        {
                                if ( old_lightids[j] == light->id )
                                {
                                        // This light also existed in the old state.
                                        // Interpolate the color.
                                        LVector3 oldcolor = LMatrix4( old_lightdata[j] ).get_row3( 3 );
                                        if ( j < old_activelights )
                                        {
                                                // Only record a match if this light was
                                                // also active in the old state.
                                                match[j] = 1;
                                        }
                                        interp_light = oldcolor;
                                        break;
                                }
                        }
                }

                // Pack the light data into a 4x4 matrix.
                LMatrix4 data = pack_lightdata( light );

                LVector3 delta( 0 );
                if ( average_lighting )
                {
                        delta = light->color - interp_light;
                        delta *= atten_factor;
                }
                data.set_row( 3, light->color - delta );

                apply_lightdatas( lights_updated, light, input, data, true );

                lights_updated++;
        }
        addlights_collector.stop();

        if ( average_lighting )
        {
                fadelights_collector.start();
                // Fade out any lights that were removed in the new state
                for ( size_t i = 0; i < old_lightcount[0]; i++ )
                {
                        if ( match[i] == 1 )
                        {
                                continue;
                        }

                        LMatrix4 data = old_lightdata[i];
                        LVector3 color = data.get_row3( 3 );
                        if ( color.length_squared() < 1.0 )
                        {
                                continue;
                        }

                        if ( lights_updated >= MAX_TOTAL_LIGHTS )
                        {
                                break;
                        }

                        light_t *oldlight = _all_lights[old_lightids[i]];

                        LVector3 new_color = color * atten_factor;
                        data.set_row( 3, new_color );

                        apply_lightdatas( lights_updated, oldlight, input, data, false );

                        lights_updated++;
                }
                fadelights_collector.stop();
        }

        input->light_count[0] = lights_updated;

        ambientboost_collector.start();
        // If we have any lights and want to do ambient boost
        if ( lights_updated > 0 && r_ambientboost.get_value() &&
                node->has_effect( AmbientBoostEffect::get_class_type() ) )
        {
                if ( pos_changed || ambientcube_changed )
                {
                        static const LVector3 lum_coeff( 0.3, 0.59, 0.11 );
                        float avg_cube_luminance = 0.0;
                        float min_cube_luminance = FLT_MAX;
                        float max_cube_luminance = 0.0;

                        // Compute average luminance of ambient cube
                        for ( int i = 0; i < 6; i++ )
                        {
                                float luminance = input->boxcolor[i].dot( lum_coeff );
                                min_cube_luminance = std::min( min_cube_luminance, luminance );
                                max_cube_luminance = std::max( max_cube_luminance, luminance );
                                avg_cube_luminance += luminance;
                        }
                        avg_cube_luminance /= 6.0;

                        // Compute the amount of direct light reaching the model
                        float direct_light = 0.0;
                        for ( int i = 0; i < lights_updated; i++ )
                        {
                                light_t *light = _all_lights[input->light_ids[i]];
                                LVector3 lvec = ( light->pos - curr_net ) * 16.0; // lighting falloff parameters in hammer space
                                float d2 = lvec.dot( lvec );
                                float d = std::sqrt( d2 );
                                float atten = 1.0;

                                float denom = light->falloff[0] +
                                        light->falloff[1] * d +
                                        light->falloff[2] * d2;
                                if ( denom > 0.00001 )
                                {
                                        atten = 1.0 / denom;
                                }

                                LVector3 lit = light->color * atten;
                                direct_light += lit.dot( lum_coeff );
                        }

                        if ( avg_cube_luminance < r_ambientmin &&
                                avg_cube_luminance < ( direct_light * r_ambientfraction.get_value() ) )
                        {
                                float boost_factor = std::min( ( direct_light * r_ambientfraction ) / max_cube_luminance,
                                        r_ambientfactor.get_value() );
                                for ( int i = 0; i < 6; i++ )
                                {
                                        input->boxcolor_boosted[i] = input->boxcolor[i] * boost_factor; // boost
                                }
                                input->ambient_boost = true;
                        }
                        else
                        {
                                input->ambient_boost = false;
                        }
                }
        }
        else
        {
                input->ambient_boost = false;
        }

        if ( input->ambient_boost )
        {
                memcpy( input->ambient_cube.p(), input->boxcolor_boosted, sizeof( LVector3 ) * 6 ); // boost
        }
        else
        {
                memcpy( input->ambient_cube.p(), input->boxcolor, sizeof( LVector3 ) * 6 ); // no boost
        }

        ambientboost_collector.stop();

        return input->state_with_input;
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

template<class T>
T AmbientProbeManager::find_closest_in_kdtree( KDTree *tree, const LPoint3 &pos,
                                               const pvector<T> &items )
{
        if ( !tree )
                return nullptr;

        std::vector<double> data = { pos[0], pos[1], pos[2] };
        auto res = tree->query( data );
        return items[res.first];
}

void AmbientProbeManager::cleanup()
{
        MutexHolder holder( _cache_mutex );

        _sunlight = nullptr;
        //_probe_kdtree = nullptr;
        //_light_kdtree = nullptr;
        _envmap_kdtree = nullptr;
        _pos_cache.clear();
        _node_data.clear();
        _probe_kdtrees.clear();
        _probes.clear();
        _all_probes.clear();
        _light_pvs.clear();
        _all_lights.clear();
        _cubemaps.clear();
}
