//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef RANGECHECKEDVAR_H
#define RANGECHECKEDVAR_H
#ifdef _WIN32
#pragma once
#endif


#include "config_bsp.h"
#include <aa_luse.h>
#include <float.h>


// Use this to disable range checks within a scope.
class EXPCL_PANDABSP CDisableRangeChecks
{
public:
	CDisableRangeChecks();
	~CDisableRangeChecks();
};


template< class T >
inline void RangeCheck( const T &value, int minValue, int maxValue )
{
}

inline void RangeCheck( const LVector3 &value, int minValue, int maxValue )
{
}


template< class T, int minValue, int maxValue, int startValue >
class CRangeCheckedVar
{
public:

	inline CRangeCheckedVar()
	{
		m_Val = startValue;
	}

	inline CRangeCheckedVar( const T &value )
	{
		*this = value;
	}

	T GetRaw() const
	{
		return m_Val;
	}

	// Clamp the value to its limits. Interpolation code uses this after interpolating.
	inline void Clamp()
	{
		if ( m_Val < minValue )
			m_Val = minValue;
		else if ( m_Val > maxValue )
			m_Val = maxValue;
	}

	inline operator const T&( ) const
	{
		return m_Val;
	}

	inline CRangeCheckedVar<T, minValue, maxValue, startValue>& operator=( const T &value )
	{
		RangeCheck( value, minValue, maxValue );
		m_Val = value;
		return *this;
	}

	inline CRangeCheckedVar<T, minValue, maxValue, startValue>& operator+=( const T &value )
	{
		return ( *this = m_Val + value );
	}

	inline CRangeCheckedVar<T, minValue, maxValue, startValue>& operator-=( const T &value )
	{
		return ( *this = m_Val - value );
	}

	inline CRangeCheckedVar<T, minValue, maxValue, startValue>& operator*=( const T &value )
	{
		return ( *this = m_Val * value );
	}

	inline CRangeCheckedVar<T, minValue, maxValue, startValue>& operator/=( const T &value )
	{
		return ( *this = m_Val / value );
	}

private:

	T m_Val;
};

#endif // RANGECHECKEDVAR_H