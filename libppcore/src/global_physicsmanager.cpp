#include "global_physicsmanager.h"

PhysicsManager GlobalPhysicsManager::physics_mgr;

PhysicsManager *GlobalPhysicsManager::get_global_ptr()
{
        return &physics_mgr;
}