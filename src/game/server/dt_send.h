/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file dt_send.h
 * @author Brian Lach
 * @date January 25, 2020
 */

#pragma once

#include "pandabase.h"
#include <pvector.h>
#include <datagram.h>

#include "config_serverdll.h"
#include "entregistry.h"

class SendProp;
class SendTable;

typedef void( *SendProxyFn )( SendProp *, void *, void *, Datagram & );
typedef void( *SendTableProxyFn )( SendProp *, unsigned char *, SendTable *, Datagram & );

enum
{
	SENDFLAGS_NONE,
};

class SendTable;

class EXPORT_SERVER_DLL SendProp
{
public:
	SendProp( const char *propname, size_t offset, size_t varsize, int bits = 0, SendProxyFn proxy = nullptr, int flags = 0 ) :
		_prop_name( propname ),
		_offset( offset ),
		_varsize( varsize ),
		_bits( bits ),
		_proxy( proxy ),
		_flags( flags ),
		_send_table( nullptr )
	{
	}

	INLINE const char *get_prop_name() const
	{
		return _prop_name;
	}

	INLINE size_t get_offset() const
	{
		return _offset;
	}

	INLINE size_t get_varsize() const
	{
		return _varsize;
	}

	INLINE SendProxyFn get_proxy() const
	{
		return _proxy;
	}

	INLINE int get_bits() const
	{
		return _bits;
	}

	INLINE int get_flags() const
	{
		return _flags;
	}

	INLINE void set_send_table( const char *table )
	{
		_send_table = table;
	}

	INLINE const char *get_send_table() const
	{
		return _send_table;
	}

private:
	const char *_prop_name;

	// Name of send table we wish to inherit from
	// when dereferencing.
	const char *_send_table;

	size_t _offset;
	size_t _varsize;
	SendProxyFn _proxy;
	int _flags;
	int _bits;
};

class EXPORT_SERVER_DLL SendTable
{
public:
	SendTable( const char *tablename = "" ) :
		_table_name( tablename ),
		_num_props( 0 )
	{
	}

	INLINE void set_table_name( const char *name )
	{
		_table_name = name;
	}

	INLINE void set_props( const pvector<SendProp> &props )
	{
		_props = props;
		_num_props = props.size();
	}

	INLINE void delete_existing_prop( const SendProp &prop )
	{
		for ( size_t i = 0; i < _props.size(); i++ )
		{
			if ( std::string( prop.get_prop_name() ) == std::string( _props[i].get_prop_name() ) )
			{
				_props.erase( _props.begin() + i );
				break;
			}

		}
	}

	INLINE void add_prop( const SendProp &prop )
	{
		delete_existing_prop( prop );

		_props.push_back( prop );
		_num_props++;
	}

	INLINE void insert_prop( const SendProp &prop )
	{
		delete_existing_prop( prop );

		_props.insert( _props.begin(), prop );
		_num_props++;
	}

	INLINE const char *get_table_name() const
	{
		return _table_name;
	}

	INLINE pvector<SendProp> &get_send_props()
	{
		return _props;
	}

	INLINE SendProp &get_prop( int i )
	{
		return _props[i];
	}

	INLINE int get_num_props() const
	{
		return _num_props;
	}

private:
	const char *_table_name;
	pvector<SendProp> _props;
	int _num_props;
};

#define BEGIN_SEND_TABLE(classname, tablename, hammername) \
BEGIN_SEND_TABLE_INTERNAL_BASE(classname) \
BEGIN_SEND_TABLE_INTERNAL(classname, tablename, hammername) \
INHERIT_SEND_TABLE(classname) \
OPEN_SEND_TABLE()

#define BEGIN_SEND_TABLE_NOBASE(classname, tablename, hammername) \
BEGIN_SEND_TABLE_INTERNAL_NOBASE(classname) \
BEGIN_SEND_TABLE_INTERNAL(classname, tablename, hammername) \
OPEN_SEND_TABLE()

#define BEGIN_SEND_TABLE_INTERNAL_BASE(classname) \
void classname::server_class_init() \
{ \
	BaseClass::server_class_init();

#define BEGIN_SEND_TABLE_INTERNAL_NOBASE(classname)	\
void classname::server_class_init() \
{

#define BEGIN_SEND_TABLE_INTERNAL(classname, tablename, hammername) \
	static bool initialized = false; \
	if ( initialized ) \
		return; \
	initialized = true; \
	std::string __hammer_name__ = #hammername; \
	classname::_server_class = ServerClass(#classname); \
	CEntRegistry::ptr()->add_network_class(&classname::_server_class); \
	typedef classname current_send_dt_class; \
	SendTable &send_table = classname::_server_class.get_send_table(); \
	send_table.set_table_name(#tablename);

#define INHERIT_SEND_TABLE(classname) \
	SendTable &base_send_table = classname::BaseClass::get_server_class_s()->get_send_table(); \
	for (size_t i = 0; i < base_send_table.get_num_props(); i++) \
	{ \
		send_table.add_prop(base_send_table.get_prop(i)); \
	}

#define OPEN_SEND_TABLE() \
	pvector<SendProp> send_props = { SendPropNull(),

#define END_SEND_TABLE() \
}; \
for (size_t i = 0; i < send_props.size(); i++) \
{ \
	SendProp &prop = send_props[i]; \
	if ( !strncmp( "__null__", prop.get_prop_name(), 8 ) ) continue; \
	send_table.add_prop(prop); \
} \
	PT( MyClass ) singleton = new MyClass; \
	singleton->ref(); \
	CEntRegistry::ptr()->link_networkname_to_class(__hammer_name__, singleton); \
}

#ifdef offsetof
#undef offsetof
#define offsetof( s, m ) ( size_t ) & ( ( (s *)0 )->m )
#endif

#define SENDINFO(varname) \
#varname, offsetof(current_send_dt_class, varname), sizeof(((current_send_dt_class *) 0)->varname)

EXPORT_SERVER_DLL void SendProxy_Int8( SendProp *prop, void *object, void *value, Datagram &dg );
EXPORT_SERVER_DLL void SendProxy_Int16( SendProp *prop, void *object, void *value, Datagram &dg );
EXPORT_SERVER_DLL void SendProxy_Int32( SendProp *prop, void *object, void *value, Datagram &dg );
EXPORT_SERVER_DLL void SendProxy_Int64( SendProp *prop, void *object, void *value, Datagram &dg );

EXPORT_SERVER_DLL void SendProxy_Uint8( SendProp *prop, void *object, void *value, Datagram &dg );
EXPORT_SERVER_DLL void SendProxy_Uint16( SendProp *prop, void *object, void *value, Datagram &dg );
EXPORT_SERVER_DLL void SendProxy_Uint32( SendProp *prop, void *object, void *value, Datagram &dg );
EXPORT_SERVER_DLL void SendProxy_Uint64( SendProp *prop, void *object, void *value, Datagram &dg );

EXPORT_SERVER_DLL void SendProxy_Float32( SendProp *prop, void *object, void *value, Datagram &dg );
EXPORT_SERVER_DLL void SendProxy_Float64( SendProp *prop, void *object, void *value, Datagram &dg );

EXPORT_SERVER_DLL void SendProxy_String( SendProp *prop, void *object, void *data, Datagram &dg );
EXPORT_SERVER_DLL void SendProxy_CString( SendProp *prop, void *object, void *data, Datagram &dg );

EXPORT_SERVER_DLL void SendTableProxy( SendProp *prop, unsigned char *object, SendTable *table, Datagram &dg );

EXPORT_SERVER_DLL SendProp SendPropNull();
EXPORT_SERVER_DLL SendProp SendPropInt( const char *varname, size_t offset, size_t varsize, int bits = 32, int flags = SENDFLAGS_NONE );
EXPORT_SERVER_DLL SendProp SendPropUint( const char *varname, size_t offset, size_t varsize, int bits = 32, int flags = SENDFLAGS_NONE );
EXPORT_SERVER_DLL SendProp SendPropFloat( const char *varname, size_t offset, size_t varsize, int bits = 32, int flags = SENDFLAGS_NONE );
EXPORT_SERVER_DLL SendProp SendPropString( const char *varname, size_t offset, size_t varsize, int flags = SENDFLAGS_NONE );
EXPORT_SERVER_DLL SendProp SendPropCString( const char *varname, size_t offset, size_t varsize, int flags = SENDFLAGS_NONE );
EXPORT_SERVER_DLL SendProp SendPropDataTable( const char *varname, const char *dtname );
EXPORT_SERVER_DLL SendProp SendPropVec3( const char *varname, size_t offset, size_t varsize, int flags = SENDFLAGS_NONE );
EXPORT_SERVER_DLL SendProp SendPropVec4( const char *varname, size_t offset, size_t varsize, int flags = SENDFLAGS_NONE );
EXPORT_SERVER_DLL SendProp SendPropVec2( const char *varname, size_t offset, size_t varsize, int flags = SENDFLAGS_NONE );

#define SendPropEntnum SendPropUint
