#ifndef GLOBAL_PARTICLEMANAGER_H
#define GLOBAL_PARTICLEMANAGER_H

#include "config_pandaplus.h"

#include <particleSystemManager.h>

class EXPCL_PANDAPLUS GlobalParticleManager
{
public:
        static ParticleSystemManager particle_mgr;
        static ParticleSystemManager *get_global_ptr();
};

#endif // GLOBAL_PARTICLEMANAGER_H