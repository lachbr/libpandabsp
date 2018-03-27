#ifndef GLOBAL_PHYSICSMANAGER_H
#define GLOBAL_PHYSICSMANAGER_H

#include "config_pandaplus.h"

#include <physicsManager.h>

class EXPCL_PANDAPLUS GlobalPhysicsManager
{
public:
        static PhysicsManager physics_mgr;
        static PhysicsManager *get_global_ptr();
};

#endif // GLOBAL_PHYSICSMANAGER_H