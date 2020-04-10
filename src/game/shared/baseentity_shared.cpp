#include "baseentity_shared.h"
#include <modelRoot.h>

IMPLEMENT_CLASS( CBaseEntityShared )

CBaseEntityShared::CBaseEntityShared() :
	_spawned( false ),
	_entnum( 0 ),
	_owner( nullptr )
{
}

void CBaseEntityShared::add_component( BaseComponentShared *component, int sort )
{
	ComponentEntry entry;
	entry.sort = sort;
	entry.component = component;

	_ordered_components.insert_nonunique( entry );
	_components[component->get_type_id()] = entry;
}

void CBaseEntityShared::init( entid_t entnum )
{
	_entnum = entnum;

	size_t count = _ordered_components.size();
	for ( size_t i = 0; i < count; i++ )
	{
		BaseComponentShared *comp = _ordered_components[i].component;
		if ( !comp->initialize() )
		{
			std::cout << "Couldn't initialize component!" << std::endl;
		}
	}
}

void CBaseEntityShared::precache()
{
	size_t count = _ordered_components.size();
	for ( size_t i = 0; i < count; i++ )
	{
		_ordered_components[i].component->precache();
	}
}

void CBaseEntityShared::spawn()
{
	size_t count = _ordered_components.size();
	for ( size_t i = 0; i < count; i++ )
	{
		BaseComponentShared *comp = _ordered_components[i].component;
		comp->spawn();
	}

	_spawned = true;
}

void CBaseEntityShared::despawn()
{
	_spawned = false;

	int count = (int)_ordered_components.size();
	for ( int i = count - 1; i >= 0; i-- )
	{
		BaseComponentShared *comp = _ordered_components[i].component;
		comp->shutdown();
	}

	_ordered_components.clear();
	_components.clear();
}

void CBaseEntityShared::simulate( double frametime )
{
	size_t count = _ordered_components.size();
	for ( size_t i = 0; i < count; i++ )
	{
		BaseComponentShared *comp = _ordered_components[i].component;
		comp->update( frametime );
	}
}
