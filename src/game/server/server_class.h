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

#include "config_serverdll.h"
#include "network_class.h"
#include "sv_entitymanager.h"

class SendProp;

typedef void( *SendProxyFn )( SendProp *, void *, void *, Datagram & );

class SendProp : public NetworkProp
{
public:
	SendProp( const std::string &propname, size_t offset, size_t varsize,
			   int bits = 0, SendProxyFn proxy = nullptr, int flags = 0 ) :
		NetworkProp( propname, offset, varsize, flags )
	{
		_proxy = proxy;
		_bits = bits;
	}

	SendProxyFn get_proxy() const
	{
		return _proxy;
	}

	int get_bits() const
	{
		return _bits;
	}

private:
	int _bits;
	SendProxyFn _proxy;
};

enum
{
	SENDFLAGS_NONE		= 0,
	SENDFLAGS_OWNRECV	= 1 << 0,
};

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

EXPORT_SERVER_DLL SendProp *SendPropNull();
EXPORT_SERVER_DLL SendProp *SendPropInt( const char *varname, size_t offset, size_t varsize, int bits = 32, int flags = SENDFLAGS_NONE );
EXPORT_SERVER_DLL SendProp *SendPropUint( const char *varname, size_t offset, size_t varsize, int bits = 32, int flags = SENDFLAGS_NONE );
EXPORT_SERVER_DLL SendProp *SendPropFloat( const char *varname, size_t offset, size_t varsize, int bits = 32, int flags = SENDFLAGS_NONE );
EXPORT_SERVER_DLL SendProp *SendPropString( const char *varname, size_t offset, size_t varsize, int flags = SENDFLAGS_NONE );
EXPORT_SERVER_DLL SendProp *SendPropCString( const char *varname, size_t offset, size_t varsize, int flags = SENDFLAGS_NONE );
EXPORT_SERVER_DLL SendProp *SendPropVec3( const char *varname, size_t offset, size_t varsize, int flags = SENDFLAGS_NONE );
EXPORT_SERVER_DLL SendProp *SendPropVec4( const char *varname, size_t offset, size_t varsize, int flags = SENDFLAGS_NONE );
EXPORT_SERVER_DLL SendProp *SendPropVec2( const char *varname, size_t offset, size_t varsize, int flags = SENDFLAGS_NONE );

#define SendPropEntnum SendPropUint

// FIXME: SendPropNull() leaks
#define OPEN_SEND_TABLE() \
	pvector<SendProp *> send_props = { SendPropNull(),

#define BEGIN_SEND_TABLE(classname) \
BEGIN_PROP_TABLE(classname, classname) \
OPEN_SEND_TABLE()

#define BEGIN_SEND_TABLE_NOBASE(classname) \
BEGIN_PROP_TABLE_NOBASE(classname, classname) \
OPEN_SEND_TABLE()

#define END_SEND_TABLE() \
}; \
for (size_t i = 0; i < send_props.size(); i++) \
{ \
	SendProp *prop = send_props[i]; \
	if ( prop->get_name() == "__null__" ) continue; \
	nclass->add_inherited_prop(prop); \
} \
ServerEntitySystem::ptr()->register_component(MyClass::ptr()); \
return 1; \
}

#define DECLARE_SERVERCLASS(classname, basename)			\
DECLARE_NETWORKCLASS(classname, basename)

#define IMPLEMENT_SERVERCLASS_ST(classname)		\
IMPLEMENT_NETWORKCLASS(classname, classname)			\
BEGIN_SEND_TABLE(classname)

#define IMPLEMENT_SERVERCLASS_ST_NOBASE(classname)	\
IMPLEMENT_NETWORKCLASS(classname, classname) \
BEGIN_SEND_TABLE_NOBASE(classname)

#define IMPLEMENT_SERVERCLASS_ST_OWNRECV(classname)		\
IMPLEMENT_NETWORKCLASS_FLAGS(classname, classname, SENDFLAGS_OWNRECV)			\
BEGIN_SEND_TABLE(classname)

#define IMPLEMENT_SERVERCLASS_ST_NOBASE_OWNRECV(classname)	\
IMPLEMENT_NETWORKCLASS_FLAGS(classname, classname, SENDFLAGS_OWNRECV) \
BEGIN_SEND_TABLE_NOBASE(classname)

#define DECLARE_ENTITY(classname, basename) \
DECLARE_CLASS(classname, basename) \
public: \
	static MyClass *ptr() \
	{ \
		static PT(MyClass) global_ptr = new MyClass; \
		return global_ptr; \
	} \
	virtual PT(CBaseEntity) make_new() \
	{ \
		return new MyClass;\
	}

#define IMPLEMENT_ENTITY(classname, editorname) \
IMPLEMENT_CLASS(classname) \
static int __link_##classname##_to_##editorname() \
{ \
	ServerEntitySystem::ptr()->link_entity_to_class(#editorname, classname::ptr()); \
	return 1; \
} \
static int __ret_##classname##_to_##editorname = __link_##classname##_to_##editorname();
