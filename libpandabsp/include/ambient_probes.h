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
        void add_node( const NodePath &node );

        void update_node( PandaNode *node );

        static CPT( ShaderAttrib ) get_identity_shattr();
        static CPT( Shader ) get_shader();

private:
        INLINE ambientprobe_t *find_closest_sample( int leaf_id, const LPoint3 &pos );

private:
        BSPLoader *_loader;

        // NodePaths to be influenced by the ambient probes.
        pvector<WeakNodePath *> _objects;
        pmap<WeakNodePath *, LPoint3> _object_pos_cache;
        pmap<int, pvector<ambientprobe_t>> _probes;
        pvector<ambientprobe_t *> _all_probes;
        pvector<PT( light_t )> _all_lights;
        pvector<pvector<light_t *>> _light_pvs;
        light_t *_sunlight;

        NodePath _vis_root;

        static CPT( Shader ) _shader;
        static CPT( ShaderAttrib ) _identity_shattr;
};

#endif // AMBIENT_PROBES_H