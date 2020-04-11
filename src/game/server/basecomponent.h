#pragma once

#include "config_serverdll.h"
#include "basecomponent_shared.h"
#include "server_class.h"
#include "networkvar.h"

class CBaseEntity;

#define MAX_CHANGED_OFFSETS	20

class EXPORT_SERVER_DLL CBaseComponent : public BaseComponentShared
{
	DECLARE_CLASS( CBaseComponent, BaseComponentShared )
public:
	CBaseComponent();

	virtual void init_mapent()
	{
	}

	void set_centity( CBaseEntity *cent )
	{
		_centity = cent;
	}

	void network_state_changed();
	void network_state_changed( void *ptr );

	bool is_property_changed( SendProp *prop );

	INLINE unsigned short *get_changed_offsets()
	{
		return _changed_offsets;
	}

	INLINE unsigned short get_change_offset( int i )
	{
		return _changed_offsets[i];
	}

	INLINE int get_num_changed_offsets() const
	{
		return _num_changed_offsets;
	}

	INLINE bool has_changed_offsets() const
	{
		return _num_changed_offsets > 0;
	}

	INLINE bool is_fully_changed() const
	{
		return get_num_changed_offsets() >= MAX_CHANGED_OFFSETS;
	}

	INLINE void mark_fully_changed()
	{
		_num_changed_offsets = MAX_CHANGED_OFFSETS;
	}

	INLINE void reset_changed_offsets()
	{
		_num_changed_offsets = 0;
	}

protected:
	unsigned short _changed_offsets[MAX_CHANGED_OFFSETS];
	int _num_changed_offsets;

	CBaseEntity *_centity;
};
