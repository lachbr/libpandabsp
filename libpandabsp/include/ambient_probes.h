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
        int leaf;
        int type;
        LVector4 direction;
        LPoint3 pos;
        LVector3 color;
        LVector4 atten;
};

#define MAXLIGHTS 2

struct nodeshaderinput_t : public ReferenceCount
{
        PTA_LVecBase3 ambient_cube;

        PTA_int light_count;
        PTA_int light_type;
        PTA_LMatrix4f light_data;

        UpdateSeq node_sequence;

        nodeshaderinput_t() :
                ReferenceCount()
        {
                ambient_cube = PTA_LVecBase3::empty_array( 6 );

                light_count = PTA_int::empty_array( 1 );
                light_type = PTA_int::empty_array( MAXLIGHTS );
                light_data = PTA_LMatrix4f::empty_array( MAXLIGHTS );
        }
};

class AmbientProbeManager
{
public:
        AmbientProbeManager();
        AmbientProbeManager( BSPLoader *loader );

        void process_ambient_probes();

        nodeshaderinput_t *update_node( PandaNode *node, CPT( TransformState ) net_ts, CPT( RenderState ) net_state );

        void cleanup();

public:
        void consider_garbage_collect();

private:
        INLINE ambientprobe_t *find_closest_sample( int leaf_id, const LPoint3 &pos );
        INLINE bool is_sky_visible( const LPoint3 &point );
        INLINE bool is_light_visible( const LPoint3 &point, const light_t *light );

        INLINE void garbage_collect_cache();

private:
        BSPLoader *_loader;

        // NodePaths to be influenced by the ambient probes.
        phash_map<WPT( PandaNode ), CPT( TransformState )> _pos_cache;
        phash_map<WPT( PandaNode ), PT( nodeshaderinput_t )> _node_data;
        phash_map<int, pvector<ambientprobe_t>> _probes;
        pvector<ambientprobe_t *> _all_probes;
        pvector<PT( light_t )> _all_lights;
        pvector<pvector<light_t *>> _light_pvs;
        light_t *_sunlight;

        NodePath _vis_root;

        UpdateSeq _node_sequence;

        double _last_garbage_collect_time;
};

#endif // AMBIENT_PROBES_H