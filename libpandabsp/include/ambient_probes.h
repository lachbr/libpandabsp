#ifndef AMBIENT_PROBES_H
#define AMBIENT_PROBES_H

#include <pvector.h>
#include <weakNodePath.h>
#include <aa_luse.h>
#include <pta_LVecBase3.h>

class BSPLoader;
struct dleafambientindex_t;
struct dleafambientlighting_t;

struct ambientprobe_t
{
        int leaf;
        LPoint3 fixed8;
        PTA_LVecBase3 cube;
};

class AmbientProbeManager
{
public:
        AmbientProbeManager();
        AmbientProbeManager( BSPLoader *loader );

        void process_ambient_probes();
        void add_node( const NodePath &node );

        void update();

private:
        INLINE ambientprobe_t *find_closest_sample( int leaf_id, const LPoint3 &pos );

private:
        BSPLoader *_loader;

        // NodePaths to be influenced by the ambient probes.
        pvector<WeakNodePath *> _objects;
        pmap<WeakNodePath *, LPoint3> _object_pos_cache;
        pmap<int, pvector<ambientprobe_t>> _probes;
};

#endif // AMBIENT_PROBES_H