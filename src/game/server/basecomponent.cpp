#include "basecomponent.h"

IMPLEMENT_CLASS( CBaseComponent )

CBaseComponent::CBaseComponent() :
	BaseComponentShared()
{
	_num_changed_offsets = MAX_CHANGED_OFFSETS;
}

/**
 * Checks if the specified SendProp has been changed since
 * the last delta snapshot. Changed properties are stored
 * by their memory offset from the start of the class.
 */
bool CBaseComponent::is_property_changed( SendProp *prop )
{
	// Assume the property has changed if the entity
	// is marked fully changed.
	if ( is_fully_changed() )
		return true;

	// If the offset of this prop is in the changed array,
	// it has been changed.
	size_t offset = prop->get_offset();
	for ( int i = 0; i < _num_changed_offsets; i++ )
		if ( _changed_offsets[i] == offset )
			return true;

	return false;
}

/**
 * Mark the entity as being fully changed.
 * All properties will be sent in the next snapshot.
 */
void CBaseComponent::network_state_changed()
{
	// Just say that we have the max number of changed offsets
	// to force each property to be sent.
	_num_changed_offsets = MAX_CHANGED_OFFSETS;
}

/**
 * Called when the value of a NetworkVar on this entity has changed.
 * We keep track of changed properties by the variable's memory offset
 * from the start of the class.
 */
void CBaseComponent::network_state_changed( void *ptr )
{
	unsigned short offset = (char *)ptr - (char *)this;

	// Already full? Don't store offset
	if ( is_fully_changed() )
		return;

	// Make sure we don't already have this offset
	for ( int i = 0; i < _num_changed_offsets; i++ )
		if ( _changed_offsets[i] == offset )
			return;

	// This property will be sent in the next delta snapshot.
	_changed_offsets[_num_changed_offsets++] = offset;
}
