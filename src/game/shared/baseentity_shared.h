#pragma once

#include "config_game_shared.h"
#include "basecomponent_shared.h"
#include "entityshared.h"
#include <ordered_vector.h>
#include <simpleHashMap.h>
#include <typedReferenceCount.h>
#include <weakPointerTo.h>

class EXPORT_GAME_SHARED CBaseEntityShared : public TypedReferenceCount
{
	DECLARE_CLASS( CBaseEntityShared, TypedReferenceCount )

public:
	CBaseEntityShared();

	void set_owner( CBaseEntityShared *owner );
	CBaseEntityShared *get_owner() const;

	entid_t get_entnum() const;

	void add_component( BaseComponentShared *component, int sort = 0 );
	BaseComponentShared *get_component( uint8_t n ) const;
	uint8_t get_num_components() const;

	template <class T>
	bool get_component( T *&component ) const;

	// Override this method to add your components!
	virtual void init( entid_t entnum );
	virtual void precache();
	virtual void spawn();

	virtual void simulate( double frametime );

	virtual void despawn();

protected:
	WPT( CBaseEntityShared ) _owner;

	bool _spawned;
	entid_t _entnum;
	
	class ComponentEntry
	{
	public:
		int sort;
		PT( BaseComponentShared ) component;

		class Sorter
		{
		public:
			constexpr bool operator()( const ComponentEntry &left, const ComponentEntry &right ) const
			{
				return left.sort < right.sort;
			}
		};
	};

	ordered_vector<ComponentEntry, ComponentEntry::Sorter> _ordered_components;
	SimpleHashMap<componentid_t, ComponentEntry, integer_hash<componentid_t>> _components;
};

inline void CBaseEntityShared::set_owner( CBaseEntityShared *owner )
{
	_owner = owner;
}

inline CBaseEntityShared *CBaseEntityShared::get_owner() const
{
	if ( !_owner.is_valid_pointer() )
	{
		return nullptr;
	}

	return _owner.p();
}

template <class T>
inline bool CBaseEntityShared::get_component( T *&component ) const
{
	component = nullptr;
	int icomp = _components.find( T::get_network_class_s()->get_id() );
	if ( icomp != -1 )
	{
		component = (T *)( _components.get_data( icomp ).component.p() );
	}

	return component != nullptr;
}

inline uint8_t CBaseEntityShared::get_num_components() const
{
	return (uint8_t)_ordered_components.size();
}

inline BaseComponentShared *CBaseEntityShared::get_component( uint8_t n ) const
{
	return _ordered_components[(size_t)n].component;
}

inline entid_t CBaseEntityShared::get_entnum() const
{
	return _entnum;
}