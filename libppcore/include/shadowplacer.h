#ifndef SHADOWPLACER_H
#define SHADOWPLACER_H

#include "config_pandaplus.h"

#include <nodePath.h>
#include <bitMask.h>
#include <collisionHandlerQueue.h>
#include <genericAsyncTask.h>

NotifyCategoryDeclNoExport( shadowPlacer );

class CollisionTraverser;

class EXPCL_PANDAPLUS ShadowPlacer
{
public:
        ShadowPlacer();
        ShadowPlacer( CollisionTraverser *c_trav, const NodePath &shadow_np,
                      const BitMask32 &floor_mask );

        void setup();

        void cleanup();

        void on();
        void off();

        void one_time_collide();

        void reset_to_origin();

private:
        static AsyncTask::DoneStatus update_task( GenericAsyncTask *task, void *data );

private:
        CollisionTraverser *_trav;
        NodePath _shadow_np;
        NodePath _ray_np;
        PT( CollisionHandlerQueue ) _lifter;
        PT( GenericAsyncTask ) _update_task;
        BitMask32 _floor_mask;
        bool _is_active;
};

#endif // SHADOWPLACER_H