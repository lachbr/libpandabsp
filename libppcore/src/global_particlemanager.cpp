#include "global_particlemanager.h"

ParticleSystemManager GlobalParticleManager::particle_mgr;

ParticleSystemManager *GlobalParticleManager::get_global_ptr()
{
        return &particle_mgr;
}