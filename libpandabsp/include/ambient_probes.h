#ifndef AMBIENT_PROBES_H
#define AMBIENT_PROBES_H

#include <pvector.h>
#include <weakNodePath.h>
#include <aa_luse.h>
#include <pta_LVecBase3.h>

class BSPLoader;
struct dleafambientindex_t;
struct dleafambientlighting_t;

enum
{
        LIGHTTYPE_SUN   = 0,
        LIGHTTYPE_POINT = 1,
        LIGHTTYPE_SPOT  = 2,
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

class AmbientProbeManager
{
public:
        AmbientProbeManager();
        AmbientProbeManager( BSPLoader *loader );

        void process_ambient_probes();

        void update_node( PandaNode *node );

        static CPT( ShaderAttrib ) get_identity_shattr();
        static CPT( Shader ) get_shader();

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
        pmap<WPT( PandaNode ), CPT( TransformState )> _pos_cache;
        pmap<int, pvector<ambientprobe_t>> _probes;
        pvector<ambientprobe_t *> _all_probes;
        pvector<PT( light_t )> _all_lights;
        pvector<pvector<light_t *>> _light_pvs;
        light_t *_sunlight;

        NodePath _vis_root;

        double _last_garbage_collect_time;

        static CPT( Shader ) _shader;
        static CPT( ShaderAttrib ) _identity_shattr;
};

#endif // AMBIENT_PROBES_H