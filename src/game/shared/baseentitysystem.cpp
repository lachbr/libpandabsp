#include "baseentitysystem.h"
#include "network_class.h"

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

void BaseEntitySystem::register_component( BaseComponentShared *singleton )
{
	componentid_t id = (componentid_t)component_registry.size();
	singleton->get_network_class()->set_id( id );

	size_t prop_count = singleton->get_network_class()->get_num_inherited_props();
	for ( size_t i = 0; i < prop_count; i++ )
	{
		NetworkProp *prop = singleton->get_network_class()->get_inherited_prop( i );
		prop->set_id( i );
	}

	component_registry[id] = singleton;
}