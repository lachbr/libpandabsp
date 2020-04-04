#include "baseentitysystem.h"

IMPLEMENT_CLASS( BaseEntitySystem )

bool BaseEntitySystem::initialize()
{
	return true;
}

void BaseEntitySystem::shutdown()
{
	size_t count = edict.size();
	for ( size_t i = 0; i < count; i++ )
	{
		edict.get_data( i )->despawn();
	}
	edict.clear();
}

void BaseEntitySystem::update( double frametime )
{
}