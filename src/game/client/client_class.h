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

#include "config_clientdll.h"
#include "network_class.h"
#include "cl_entitymanager.h"


// ------------------------------------------------------------------
// Recv Code
// ------------------------------------------------------------------

class RecvProp;

typedef void( *RecvProxyFn )( RecvProp *prop, void *object, void *out, DatagramIterator &dgi );

class RecvProp : public NetworkProp
{
public:
        RecvProp( const std::string &propname, size_t offset, size_t varsize,
                  RecvProxyFn proxy = nullptr, int flags = 0 ) :
                NetworkProp( propname, offset, varsize, flags )
        {
                _proxy = proxy;
        }

        RecvProxyFn get_proxy() const
        {
                return _proxy;
        }

private:
        RecvProxyFn _proxy;
};

enum
{
        RECVFLAGS_NONE = 0,
        RECVFLAGS_PRESPAWN = 1 << 0,
        RECVFLAGS_POSTSPAWN = 1 << 1,
};

EXPORT_CLIENT_DLL void RecvProxy_Int8( RecvProp *prop, void *object, void *out, DatagramIterator &dgi );
EXPORT_CLIENT_DLL void RecvProxy_Int16( RecvProp *prop, void *object, void *out, DatagramIterator &dgi );
EXPORT_CLIENT_DLL void RecvProxy_Int32( RecvProp *prop, void *object, void *out, DatagramIterator &dgi );
EXPORT_CLIENT_DLL void RecvProxy_Int64( RecvProp *prop, void *object, void *out, DatagramIterator &dgi );

EXPORT_CLIENT_DLL void RecvProxy_Uint8( RecvProp *prop, void *object, void *out, DatagramIterator &dgi );
EXPORT_CLIENT_DLL void RecvProxy_Uint16( RecvProp *prop, void *object, void *out, DatagramIterator &dgi );
EXPORT_CLIENT_DLL void RecvProxy_Uint32( RecvProp *prop, void *object, void *out, DatagramIterator &dgi );
EXPORT_CLIENT_DLL void RecvProxy_Uint64( RecvProp *prop, void *object, void *out, DatagramIterator &dgi );

EXPORT_CLIENT_DLL void RecvProxy_Float32( RecvProp *prop, void *object, void *out, DatagramIterator &dgi );
EXPORT_CLIENT_DLL void RecvProxy_Float64( RecvProp *prop, void *object, void *out, DatagramIterator &dgi );

EXPORT_CLIENT_DLL void RecvProxy_String( RecvProp *prop, void *object, void *out, DatagramIterator &dgi );
EXPORT_CLIENT_DLL void RecvProxy_CString( RecvProp *prop, void *object, void *out, DatagramIterator &dgi );

template <class T>
EXPORT_CLIENT_DLL void RecvProxy_Vec( RecvProp *prop, void *object, void *out, DatagramIterator &dgi );

EXPORT_CLIENT_DLL RecvProp *RecvPropNull();
EXPORT_CLIENT_DLL RecvProp *RecvPropInt( const char *varname, size_t offset, size_t varsize, RecvProxyFn proxy = nullptr, int bits = 32, int flags = RECVFLAGS_NONE );
EXPORT_CLIENT_DLL RecvProp *RecvPropUint( const char *varname, size_t offset, size_t varsize, RecvProxyFn proxy = nullptr, int bits = 32, int flags = RECVFLAGS_NONE );
EXPORT_CLIENT_DLL RecvProp *RecvPropFloat( const char *varname, size_t offset, size_t varsize, RecvProxyFn proxy = nullptr, int bits = 32, int flags = RECVFLAGS_NONE );
EXPORT_CLIENT_DLL RecvProp *RecvPropString( const char *varname, size_t offset, size_t varsize, RecvProxyFn proxy = nullptr, int flags = RECVFLAGS_NONE );
EXPORT_CLIENT_DLL RecvProp *RecvPropCString( const char *varname, size_t offset, size_t varsize, RecvProxyFn proxy = nullptr, int flags = RECVFLAGS_NONE );
EXPORT_CLIENT_DLL RecvProp *RecvPropVec4( const char *varname, size_t offset, size_t varsize, RecvProxyFn proxy = nullptr, int flags = RECVFLAGS_NONE );
EXPORT_CLIENT_DLL RecvProp *RecvPropVec3( const char *varname, size_t offset, size_t varsize, RecvProxyFn proxy = nullptr, int flags = RECVFLAGS_NONE );
EXPORT_CLIENT_DLL RecvProp *RecvPropVec2( const char *varname, size_t offset, size_t varsize, RecvProxyFn proxy = nullptr, int flags = RECVFLAGS_NONE );

#define RecvPropEntnum RecvPropUint

// FIXME: RecvPropNull() leaks
#define OPEN_RECV_TABLE() \
	pvector<RecvProp *> recv_props = { RecvPropNull(),

#define BEGIN_RECV_TABLE(classname, networkname) \
BEGIN_PROP_TABLE(classname, networkname) \
OPEN_RECV_TABLE()

#define BEGIN_RECV_TABLE_NOBASE(classname, networkname) \
BEGIN_PROP_TABLE_NOBASE(classname, networkname) \
OPEN_RECV_TABLE()

#define END_RECV_TABLE() \
}; \
for (size_t i = 0; i < recv_props.size(); i++) \
{ \
	RecvProp *prop = recv_props[i]; \
	if ( prop->get_name() == "__null__" ) continue; \
	nclass->add_inherited_prop(prop); \
} \
 ClientEntitySystem::ptr()->link_networkname_to_entity(__networkname, MyClass::ptr()); \
 return 1; \
} \

#define DECLARE_CLIENTCLASS(classname, basename)			\
DECLARE_NETWORKCLASS(classname, basename)

#define IMPLEMENT_CLIENTCLASS_RT(classname, networkname)	\
IMPLEMENT_NETWORKCLASS(classname, networkname)				\
BEGIN_RECV_TABLE(classname, networkname)

#define IMPLEMENT_CLIENTCLASS_RT_NOBASE(classname, networkname)	\
IMPLEMENT_NETWORKCLASS(classname, networkname)	\
BEGIN_RECV_TABLE_NOBASE(classname, networkname)