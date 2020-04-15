/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file keyvalues.h
 * @author Brian Lach
 * @date April 15, 2020
 *
 * @desc The new and much cleaner interface for dealing with Valve's KeyValue
 *	 format. KeyValues can either be created procedurally or loaded from
	 disk.
 */

#pragma once

#include "config_vifparser.h"

#include "referenceCount.h"
#include "pointerTo.h"
#include "simpleHashMap.h"
#include "luse.h"

class CKeyValuesTokenizer;

/**
 * Represents a single block from a key-values file.
 * Has a map of string key-value pairs, and can have a list of child blocks.
 */
class EXPCL_VIF CKeyValues : public ReferenceCount
{
public:
	CKeyValues( const std::string &name, CKeyValues *parent = nullptr );

	//void set_parent( CKeyValues *parent );
	CKeyValues *get_parent() const;

	void set_name( const std::string &name );
	const std::string &get_name() const;

	void add_child( CKeyValues *child );
	CKeyValues *get_child( size_t n ) const;
	int find_child( const std::string &name ) const;
	pvector<CKeyValues *> get_children_with_name( const std::string &name ) const;
	size_t get_num_children() const;

	std::string &operator []( const std::string &key );

	size_t get_num_keys() const;
	bool has_key( const std::string &name ) const;
	int find_key( const std::string &name ) const;
	const std::string &get_key( size_t n ) const;
	const std::string &get_value( size_t n ) const;

	const Filename &get_filename() const;

private:
	void parse( CKeyValuesTokenizer *tokenizer );

public:
	static PT( CKeyValues ) load( const Filename &filename );

	static pvector<int> parse_num_list_str( const std::string &str );
	static pvector<float> parse_float_list_str( const std::string &str );
	static pvector<pvector<int>> parse_int_tuple_list_str( const std::string &str );
	static pvector<pvector<float>> parse_num_array_str( const std::string &str );
	static LVecBase3f to_3f( const std::string &str );
	static LVecBase2f to_2f( const std::string &str );
	static LVecBase4f to_4f( const std::string &str );

	template<class T>
	static std::string to_string( T v );

	template<class T>
	static std::string to_string( const pvector<T> &v );

	static std::string to_string( const LVecBase3f &v );
	static std::string to_string( const LVecBase4f &v );
	static std::string to_string( const LVecBase2f &v );

private:
	PT( CKeyValues ) _parent;
	Filename _filename;
	std::string _name;
	SimpleHashMap<std::string, std::string, string_hash> _keyvalues;
	pvector<PT( CKeyValues )> _children;
};

inline CKeyValues::CKeyValues( const std::string &name, CKeyValues *parent )
{
	_name = name;
	_parent = parent;
}

//inline void CKeyValues::set_parent( CKeyValues *parent )
//{
//	_parent = parent;
//}

inline CKeyValues *CKeyValues::get_parent() const
{
	return _parent;
}

inline void CKeyValues::set_name( const std::string &name )
{
	_name = name;
}

inline const std::string &CKeyValues::get_name() const
{
	return _name;
}

inline void CKeyValues::add_child( CKeyValues *child )
{
	child->_parent = this;
	_children.push_back( child );
}

inline CKeyValues *CKeyValues::get_child( size_t n ) const
{
	return _children[n];
}

inline size_t CKeyValues::get_num_children() const
{
	return _children.size();
}

inline std::string &CKeyValues::operator[]( const std::string &key )
{
	return _keyvalues[key];
}

inline size_t CKeyValues::get_num_keys() const
{
	return _keyvalues.size();
}

inline bool CKeyValues::has_key( const std::string &key ) const
{
	return _keyvalues.find( key ) != -1;
}

inline int CKeyValues::find_key( const std::string &key ) const
{
	return _keyvalues.find( key );
}

inline const std::string &CKeyValues::get_key( size_t n ) const
{
	return _keyvalues.get_key( n );
}

inline const std::string &CKeyValues::get_value( size_t n ) const
{
	return _keyvalues.get_data( n );
}

inline const Filename &CKeyValues::get_filename() const
{
	return _filename;
}
