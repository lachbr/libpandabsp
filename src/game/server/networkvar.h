/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file networkvar.h
 * @author Brian Lach
 * @date January 24, 2020
 */

#pragma once

#include "config_serverdll.h"

template< class T, class Changer >
class EXPORT_SERVER_DLL BaseNetworkVar
{
public:
	template <class C>
	const T &operator=( const C &val )
	{
		return set( (const T)val );
	}

	template <class C>
	const T &operator=( const BaseNetworkVar<C, Changer> &val )
	{
		return set( (const T)val._value );
	}

	const T &set( const T &val )
	{
		if ( memcmp( &_value, &val, sizeof( T ) ) )
		{
			network_state_changed();
			_value = val;
		}
		return _value;
	}

	T &get_for_modify()
	{
		network_state_changed();
		return _value;
	}

	template <class C>
	const T &operator+=( const C &val )
	{
		return set( _value + (const T)val );
	}

	template <class C>
	const T &operator-=( const C &val )
	{
		return set( _value - (const T)val );
	}


	const T *operator->() const
	{
		return &_value;
	}

	const T &get() const
	{
		return _value;
	}

	operator const T &( ) const
	{
		return _value;
	}

	template<class C>
	const T &operator&=( const C &val )
	{
		return set( _value & (const T)val );
	}

	template<class C>
	const T &operator/=( const C &val )
	{
		return set( _value / (const T)val );
	}

	template<class C>
	const T &operator*=( const C &val )
	{
		return set( _value * (const T)val );
	}

	template<class C>
	const T &operator^=( const C &val )
	{
		return set( _value ^ (const T)val );
	}

	template<class C>
	const T &operator|=( const C &val )
	{
		return set( _value | (const T)val );
	}

	T operator--()
	{
		return ( *this -= 1 );
	}

	const T &operator++()
	{
		return ( *this += 1 );
	}

	T operator++( int ) // postfix version
	{
		T val = _value;
		( *this += 1 );
		return val;
	}

	T operator--( int ) // postfix version
	{
		T val = _value;
		( *this -= 1 );
		return val;
	}

	T _value;

protected:
	INLINE void network_state_changed()
	{
		Changer::network_state_changed( this );
	}
};

template <class T, class Changer>
class BaseNetworkVector : public BaseNetworkVar<T, Changer>
{
public:
	const T &operator=( const T &new_val )
	{
		return BaseNetworkVar<T, Changer>::set( new_val );
	}
};

#define MyOffsetOf( type, var ) ( (size_t)&( (type *)0 )->var )

#define NETWORK_VAR_START(type, name)	\
	class NetworkVar_##name;	\
	friend class NetworkVar_##name;	\
	typedef MyClass MakeANetworkVar_##name;	\
	class NetworkVar_##name		\
	{				\
	public:	\
		template <typename T>	\
		friend int server_class_init( T * );

#define NETWORK_VAR_END(type, name, base, state_changed_fn)	\
public:			\
	static INLINE void network_state_changed(void *ptr)	\
	{	\
		((MyClass *)( (char *)ptr - MyOffsetOf( MyClass, name ) )) \
		->state_changed_fn(ptr);	\
	} \
	} \
	;	\
	base<type, NetworkVar_##name> name;

#define NetworkVar(type, name)	\
	NETWORK_VAR_START(type, name)	\
	NETWORK_VAR_END(type, name, BaseNetworkVar, network_state_changed)

#define NetworkString(name) NetworkVar(std::string, name)
#define NetworkInt(name) NetworkVar(int, name)
#define NetworkUint(name) NetworkVar(unsigned int, name)
#define NetworkFloat(name) NetworkVar(float, name)
#define NetworkDouble(name) NetworkVar(double, name)

#define NetworkVec4(name)	\
	NETWORK_VAR_START(LVector4f, name)	\
	NETWORK_VAR_END(LVector4f, name, BaseNetworkVector, network_state_changed)

#define NetworkVec3(name)	\
	NETWORK_VAR_START(LVector3f, name)	\
	NETWORK_VAR_END(LVector3f, name, BaseNetworkVector, network_state_changed)

#define NetworkVec2(name)	\
	NETWORK_VAR_START(LVector2f, name)	\
	NETWORK_VAR_END(LVector2f, name, BaseNetworkVector, network_state_changed)


