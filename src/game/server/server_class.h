/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file server_class.h
 * @author Brian Lach
 * @date January 25, 2020
 */

#pragma once

#include "dt_send.h"

class EXPORT_SERVER_DLL ServerClass
{
public:
	ServerClass() :
		_class_id( 0 ),
		_network_name( nullptr )
	{
	}

	ServerClass( const char *network_name ) :
		_class_id( 0 ),
		_network_name( network_name )
	{
	}

	INLINE void set_send_table( const SendTable &table )
	{
		_send_table = table;
	}

	INLINE SendTable &get_send_table()
	{
		return _send_table;
	}

	INLINE const char *get_network_name() const
	{
		return _network_name;
	}

	INLINE void set_class_id( int id )
	{
		_class_id = id;
	}

	INLINE int get_class_id() const
	{
		return _class_id;
	}

private:
	SendTable _send_table;
	const char *_network_name;
	int _class_id;
};

// This can be used to give all datatables access to protected and private
// members of the class.
#define ALLOW_DATATABLES_PRIVATE_ACCESS() \
	template <typename T>             \
	friend int server_class_init( T* );

#define DECLARE_SERVERCLASS_NOBASE ALLOW_DATATABLES_PRIVATE_ACCESS

#define DECLARE_SERVERCLASS()			\
private:					\
	static ServerClass _server_class;	\
public:						\
	virtual ServerClass *get_server_class()	\
	{					\
		return &_server_class;		\
	}					\
	static ServerClass *get_server_class_s()	\
	{					\
		return &_server_class;		\
	}					\
	virtual PT(CBaseEntity) make_new() \
	{ \
		return new MyClass; \
	} \
	static void server_class_init(); \
	DECLARE_SERVERCLASS_NOBASE()

#define IMPLEMENT_SERVERCLASS(classname)	\
IMPLEMENT_CLASS(classname)					\
ServerClass classname::_server_class;

#define IMPLEMENT_SERVERCLASS_ST(classname, tablename, hammername)		\
IMPLEMENT_SERVERCLASS(classname)			\
BEGIN_SEND_TABLE(classname, tablename, hammername)

#define IMPLEMENT_SERVERCLASS_ST_NOBASE(classname, tablename, hammername)	\
IMPLEMENT_SERVERCLASS(classname) \
BEGIN_SEND_TABLE_NOBASE(classname, tablename, hammername)