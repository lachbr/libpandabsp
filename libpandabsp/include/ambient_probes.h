/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file ambient_probes.h
 * @author Brian Lach
 * @date August 03, 2018
 */

#ifndef AMBIENT_PROBES_H
#define AMBIENT_PROBES_H

#include <pvector.h>
#include <weakNodePath.h>
#include <aa_luse.h>
#include <pta_LVecBase3.h>
#include <colorAttrib.h>
#include <texGenAttrib.h>
#include <lightRampAttrib.h>
#include <cullableObject.h>
#include <shaderAttrib.h>
#include <updateSeq.h>

#include <unordered_map>
#include <bitset>

class BSPLoader;
struct dleafambientindex_t;
struct dleafambientlighting_t;

enum
{
        LIGHTTYPE_SUN           = 0,
        LIGHTTYPE_POINT         = 1,
        LIGHTTYPE_SPHERE        = 2,
        LIGHTTYPE_SPOT          = 3,
};

struct ambientprobe_t
{
        int leaf;
        LPoint3 pos;
        PTA_LVecBase3 cube;
        NodePath visnode;
};

struct light_t : public ReferenceCount
{
        int id;
        int leaf;
        int type;
        LVector4 direction;
        LPoint3 pos;
        LVector3 color;
        LVector4 atten;

        // If the light is potentially visible, updated each frame.
        LVector4 eye_pos;
        LVector4 eye_direction;
};

// Max stationary lights
#define MAX_ACTIVE_LIGHTS 4
// Max lights we could be dimming
#define MAX_TOTAL_LIGHTS 8

#define LIGHTING_UNINITIALIZED -1

#ifndef CPPPARSER
struct nodeshaderinput_t : public ReferenceCount
{
        PTA_LVecBase3 ambient_cube;

        PTA_int light_count;
        PTA_int light_type;
        PTA_LMatrix4f light_data;

        PN_stdfloat lighting_time;
        PTA_int light_ids;

        UpdateSeq node_sequence;

        ambientprobe_t *amb_probe;
        pvector<light_t *> locallights;
        int sky_idx;
        std::bitset<0xFFF> occluded_lights;

        int active_lights;

        nodeshaderinput_t() :
                ReferenceCount()
        {
                ambient_cube = PTA_LVecBase3::empty_array( 6 );

                light_count = PTA_int::empty_array( 1 );
                light_type = PTA_int::empty_array( MAX_TOTAL_LIGHTS );
                light_data = PTA_LMatrix4f::empty_array( MAX_TOTAL_LIGHTS );
                light_ids = PTA_int::empty_array( MAX_TOTAL_LIGHTS );
                amb_probe = nullptr;
                sky_idx = -1;
                active_lights = 0;

                lighting_time = LIGHTING_UNINITIALIZED;
        }

        nodeshaderinput_t( const nodeshaderinput_t &other ) :
                ReferenceCount(),
                lighting_time( other.lighting_time ),
                node_sequence( other.node_sequence ),
                amb_probe( other.amb_probe ),
                locallights( other.locallights ),
                sky_idx( other.sky_idx ),
                active_lights( other.active_lights ),
                occluded_lights( other.occluded_lights )
        {
                //ambient_cube = PTA_LVecBase3::empty_array( 6 );
                //light_count = PTA_int::empty_array( 1 );
                //light_type = PTA_int::empty_array( MAX_TOTAL_LIGHTS );
                //light_data = PTA_LMatrix4f::empty_array( MAX_TOTAL_LIGHTS );
                //light_ids = PTA_int::empty_array( MAX_TOTAL_LIGHTS );

                light_type.set_data( other.light_type.get_data() );
                ambient_cube.set_data( other.ambient_cube.get_data() );
                light_type.set_data( other.light_type.get_data() );
                light_data.set_data( other.light_data.get_data() );
                light_ids.set_data( other.light_ids.get_data() );
                light_count.set_data( other.light_count.get_data() );
        }
};
#else
struct nodeshaderinput_t;
#endif

class AmbientProbeManager
{
public:
        AmbientProbeManager();
        AmbientProbeManager( BSPLoader *loader );

        void process_ambient_probes();

        PT( nodeshaderinput_t ) update_node( PandaNode *node, const TransformState *net_ts );

        void cleanup();

public:
        void consider_garbage_collect();

        void xform_lights( const TransformState *cam_trans );

private:
        INLINE ambientprobe_t *find_closest_sample( int leaf_id, const LPoint3 &pos );
        INLINE bool is_sky_visible( const LPoint3 &point );
        INLINE bool is_light_visible( const LPoint3 &point, const light_t *light );

        INLINE void garbage_collect_cache();

private:
        BSPLoader *_loader;

        // NodePaths to be influenced by the ambient probes.
        SimpleHashMap<WPT( PandaNode ), CPT( TransformState )> _pos_cache;
        SimpleHashMap<WPT( PandaNode ), PT( nodeshaderinput_t )> _node_data;
        SimpleHashMap<int, pvector<ambientprobe_t>, int_hash> _probes;
        pvector<ambientprobe_t *> _all_probes;
        pvector<PT( light_t )> _all_lights;
        pvector<pvector<light_t *>> _light_pvs;
        light_t *_sunlight;

        NodePath _vis_root;

        UpdateSeq _node_sequence;

        double _last_garbage_collect_time;
};

#endif // AMBIENT_PROBES_H