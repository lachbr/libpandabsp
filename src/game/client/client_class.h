/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file client_class.h
 * @author Brian Lach
 * @date January 25, 2020
 */

#pragma once

#include "pandabase.h"
#include "dt_recv.h"

// -------------------------------------------
// Client class
// -------------------------------------------

class EXPORT_CLIENT_DLL ClientClass
{
public:
	ClientClass() :
		_class_id( 0 ),
		_network_name( nullptr )
	{
	}

	ClientClass( const char *network_name ) :
		_class_id( 0 ),
		_network_name( network_name )
	{
	}

	INLINE void set_recv_table( const RecvTable &table )
	{
		_recv_table = table;
	}

	INLINE RecvTable &get_recv_table()
	{
		return _recv_table;
	}

	INLINE void set_class_id( int class_id )
	{
		_class_id = class_id;
	}

	INLINE int get_class_id() const
	{
		return _class_id;
	}

	INLINE const char *get_network_name() const
	{
		return _network_name;
	}

private:
	RecvTable _recv_table;
	int _class_id;
	const char *_network_name;
};

// This can be used to give all datatables access to protected and private
// members of the class.
#define ALLOW_DATATABLES_PRIVATE_ACCESS() \
	template <typename T>             \
	friend int client_class_init( T* );

#define DECLARE_CLIENTCLASS_NOBASE ALLOW_DATATABLES_PRIVATE_ACCESS

#define DECLARE_CLIENTCLASS()			\
private:					\
	static ClientClass _client_class;	\
public:						\
	static ClientClass *get_client_class_s()\
	{					\
		return &_client_class;		\
	}					\
	virtual ClientClass *get_client_class() \
	{ \
		return &_client_class; \
	} \
	virtual PT( C_BaseEntity ) make_new() \
	{ \
		return new MyClass; \
	} \
	static void client_class_init(); \
	DECLARE_CLIENTCLASS_NOBASE()

#define IMPLEMENT_CLIENTCLASS(classname, networkname)		\
IMPLEMENT_CLASS(classname)					\
ClientClass classname::_client_class;		\

#define IMPLEMENT_CLIENTCLASS_RT(classname, tablename, networkname)	\
IMPLEMENT_CLIENTCLASS(classname, networkname)				\
BEGIN_RECV_TABLE(classname, tablename, networkname)

#define IMPLEMENT_CLIENTCLASS_RT_NOBASE(classname, tablename, networkname)	\
IMPLEMENT_CLIENTCLASS(classname, networkname)	\
BEGIN_RECV_TABLE_NOBASE(classname, tablename, networkname)