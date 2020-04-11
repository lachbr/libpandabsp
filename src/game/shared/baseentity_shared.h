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

	void add_component( BaseComponentShared *component );
	void add_component( BaseComponentShared *component, int sort );
	BaseComponentShared *get_component( uint8_t n ) const;
	BaseComponentShared *get_component_by_id( componentid_t id ) const;
	uint8_t get_num_components() const;

	template <class T>
	bool get_component( T *&component ) const;

	template <class T>
	bool get_component_of_type( T *&component ) const;

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

	int _curr_sort;

	ordered_vector<ComponentEntry, ComponentEntry::Sorter> _ordered_components;
	SimpleHashMap<componentid_t, ComponentEntry, integer_hash<componentid_t>> _components;
};

inline void CBaseEntityShared::add_component( BaseComponentShared *component )
{
	add_component( component, _curr_sort++ );
}

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

template<class Type>
inline bool CBaseEntityShared::get_component_of_type( Type *&system ) const
{
	system = nullptr;

	size_t count = _ordered_components.size();
	for ( size_t i = 0; i < count; i++ )
	{
		ComponentEntry entry = _ordered_components[i];
		if ( entry.component->is_of_type( Type::get_class_type() ) )
		{
			system = (Type *)( entry.component.p() );
		}
	}

	return system != nullptr;
}

inline uint8_t CBaseEntityShared::get_num_components() const
{
	return (uint8_t)_ordered_components.size();
}

inline BaseComponentShared *CBaseEntityShared::get_component( uint8_t n ) const
{
	return _ordered_components[(size_t)n].component;
}

inline BaseComponentShared *CBaseEntityShared::get_component_by_id( componentid_t id ) const
{
	int icomp = _components.find( id );
	if ( icomp != -1 )
	{
		return _components.get_data( icomp ).component;
	}

	return nullptr;
}

inline entid_t CBaseEntityShared::get_entnum() const
{
	return _entnum;
}