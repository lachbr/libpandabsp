#pragma once

#include <pvector.h>
#include <vector_string.h>
#include <stl_compares.h>

#include "basecomponent_shared.h"

class NetworkProp
{
public:
	NetworkProp( const std::string &name, size_t offset, size_t varsize, int flags = 0 )
	{
		_name = name;
		_offset = offset;
		_varsize = varsize;
		_flags = flags;
		_id = 0u;
	}

	void set_id( uint8_t id )
	{
		_id = id;
	}

	uint8_t get_id() const
	{
		return _id;
	}

	size_t get_hash( size_t hash ) const
	{
		hash = string_hash::add_hash( hash, _name );
		hash = size_t_hash::add_hash( hash, _varsize );
		return hash;
	}

	const std::string &get_name() const
	{
		return _name;
	}

	size_t get_offset() const
	{
		return _offset;
	}

	size_t get_varsize() const
	{
		return _varsize;
	}

	size_t get_flags() const
	{
		return _flags;
	}

protected:
	std::string _name;
	uint8_t _id;
	// Byte offset into the class
	size_t _offset;
	size_t _varsize;
	int _flags;
};

typedef pvector<NetworkProp *> NetworkProps;

class NetworkClass
{
public:
	NetworkClass( const std::string &name )
	{
		_name = name;
		_id = 0u;
	}

	void set_id( componentid_t id )
	{
		_id = id;
	}

	componentid_t get_id() const
	{
		return _id;
	}

	// Used to validate client/server classes.
	size_t get_hash() const
	{
		size_t hash = 0u;

		hash = string_hash::add_hash( hash, _name );

		size_t prop_count = get_num_inherited_props();
		for ( size_t i = 0; i < prop_count; i++ )
		{
			hash = get_inherited_prop( i )->get_hash( hash );
		}

		return hash;
	}

	void add_inherited_prop( NetworkProp *prop )
	{
		_inherited_props.push_back( prop );
	}

	NetworkProp *get_inherited_prop( size_t n ) const
	{
		return _inherited_props[n];
	}

	size_t get_num_inherited_props() const
	{
		return _inherited_props.size();
	}

	const std::string &get_name() const
	{
		return _name;
	}

	NetworkProps::const_iterator find_inherited_prop( const std::string &propname ) const
	{
		return std::find_if( _inherited_props.begin(), _inherited_props.end(),
				     [propname]( NetworkProp *const other )
		{
			return propname == other->get_name();
		} );
	}

	NetworkProps::const_iterator inherited_props_end() const
	{
		return _inherited_props.end();
	}

protected:
	std::string _name;
	componentid_t _id;
	// Props part of this class and inherited
	NetworkProps _inherited_props;
};

#ifdef offsetof
#undef offsetof
#define offsetof( s, m ) ( size_t ) & ( ( (s *)0 )->m )
#endif

#define PROPINFO(varname) \
#varname, offsetof(current_dt_class, varname), sizeof(((current_dt_class *) 0)->varname)

#define DECLARE_NETWORKCLASS(classname, basename) \
DECLARE_CLASS(classname, basename) \
public:						\
	virtual NetworkClass *get_network_class()	\
	{					\
		return get_network_class_s();		\
	}					\
	virtual componentid_t get_type_id() \
	{ \
		return get_network_class_s()->get_id(); \
	} \
	static NetworkClass *get_network_class_s();			\
	static MyClass *ptr() \
	{ \
	 	static PT(MyClass) global_ptr = new MyClass; \
		return global_ptr; \
	} \
	virtual PT(BaseComponentShared) make_new() \
	{ \
		return new MyClass; \
	} \
	static int network_class_init();

#define IMPLEMENT_NETWORKCLASS(classname, networkname) \
IMPLEMENT_CLASS(classname) \
NetworkClass *classname::get_network_class_s() \
{ \
	static NetworkClass *global_ptr = new NetworkClass(#networkname); \
	return global_ptr; \
} \
static int __implement_##classname##_ret = classname::network_class_init(); \

#define INHERIT_PROP_TABLE(classname) \
	NetworkClass *base_nclass = classname::BaseClass::get_network_class_s(); \
	for (size_t i = 0; i < base_nclass->get_num_inherited_props(); i++) \
	{ \
		nclass->add_inherited_prop(base_nclass->get_inherited_prop(i)); \
	}

#define BEGIN_PROP_TABLE_INTERNAL_BASE(classname) \
int classname::network_class_init() \
{ \
	BaseClass::network_class_init();

#define BEGIN_PROP_TABLE_INTERNAL_NOBASE(classname)	\
int classname::network_class_init() \
{

#define BEGIN_PROP_TABLE_INTERNAL(classname, networkname) \
	static bool initialized = false; \
	if ( initialized ) \
		return 0; \
	initialized = true; \
	NetworkClass *nclass = classname::get_network_class_s(); \
	std::string __classname = #classname; \
	std::string __networkname = #networkname; \
	typedef classname current_dt_class;

#define BEGIN_PROP_TABLE(classname, networkname) \
BEGIN_PROP_TABLE_INTERNAL_BASE(classname) \
BEGIN_PROP_TABLE_INTERNAL(classname, networkname) \
INHERIT_PROP_TABLE(classname)

#define BEGIN_PROP_TABLE_NOBASE(classname, networkname) \
BEGIN_PROP_TABLE_INTERNAL_NOBASE(classname) \
BEGIN_PROP_TABLE_INTERNAL(classname, networkname)