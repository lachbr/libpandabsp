#include "server_class.h"

#include <aa_luse.h>

void SendProxy_Int8( SendProp *prop, void *object, void *data, Datagram &dg )
{
	dg.add_int8( ( *(int8_t *)data ) );
}

void SendProxy_Int16( SendProp *prop, void *object, void *data, Datagram &dg )
{
	dg.add_int16( ( *(int16_t *)data ) );
}

void SendProxy_Int32( SendProp *prop, void *object, void *data, Datagram &dg )
{
	dg.add_int32( ( *(int32_t *)data ) );
}

void SendProxy_Int64( SendProp *prop, void *object, void *data, Datagram &dg )
{
	dg.add_int64( ( *(int64_t *)data ) );
}

void SendProxy_Uint8( SendProp *prop, void *object, void *data, Datagram &dg )
{
	dg.add_uint8( ( *(uint8_t *)data ) );
}

void SendProxy_Uint16( SendProp *prop, void *object, void *data, Datagram &dg )
{
	dg.add_uint16( ( *(uint16_t *)data ) );
}

void SendProxy_Uint32( SendProp *prop, void *object, void *data, Datagram &dg )
{
	dg.add_uint32( ( *(uint32_t *)data ) );
}

void SendProxy_Uint64( SendProp *prop, void *object, void *data, Datagram &dg )
{
	dg.add_uint64( ( *(uint64_t *)data ) );
}

void SendProxy_Float32( SendProp *prop, void *object, void *data, Datagram &dg )
{
	dg.add_float32( ( *(PN_float32 *)data ) );
}

void SendProxy_Float64( SendProp *prop, void *object, void *data, Datagram &dg )
{
	dg.add_float64( ( *(PN_float64 *)data ) );
}

void SendProxy_String( SendProp *prop, void *object, void *data, Datagram &dg )
{
	dg.add_string( *( std::string * )data );
}

void SendProxy_CString( SendProp *prop, void *object, void *data, Datagram &dg )
{
	dg.add_string( std::string( (char *)data ) );
}

SendProp *SendPropNull()
{
	return new SendProp( "__null__", 0, 0, 0, nullptr, 0 );
}

SendProp *SendPropInt( const char *varname, size_t offset, size_t varsize, int bits, int flags )
{
	SendProxyFn proxy;
	switch ( bits )
	{
	case 8:
		proxy = SendProxy_Int8;
		break;
	case 16:
		proxy = SendProxy_Int16;
		break;
	case 32:
		proxy = SendProxy_Int32;
		break;
	case 64:
		proxy = SendProxy_Int64;
		break;
	default:
		proxy = SendProxy_Int32;
	}

	return new SendProp( varname, offset, varsize, bits, proxy, flags );
}

SendProp *SendPropUint( const char *varname, size_t offset, size_t varsize, int bits, int flags )
{
	SendProxyFn proxy;
	switch ( bits )
	{
	case 8:
		proxy = SendProxy_Uint8;
		break;
	case 16:
		proxy = SendProxy_Uint16;
		break;
	case 32:
		proxy = SendProxy_Uint32;
		break;
	case 64:
		proxy = SendProxy_Uint64;
		break;
	default:
		proxy = SendProxy_Uint32;
	}

	return new SendProp( varname, offset, varsize, bits, proxy, flags );
}

SendProp *SendPropFloat( const char *varname, size_t offset, size_t varsize, int bits, int flags )
{
	SendProxyFn proxy;
	switch ( bits )
	{
	case 32:
		proxy = SendProxy_Float32;
		break;
	case 64:
		proxy = SendProxy_Float64;
		break;
	default:
		proxy = SendProxy_Float32;
		break;
	}

	return new SendProp( varname, offset, varsize, bits, proxy, flags );
}

SendProp *SendPropString( const char *varname, size_t offset, size_t varsize, int flags )
{
	return new SendProp( varname, offset, varsize, 0, SendProxy_String, flags );
}

SendProp *SendPropCString( const char *varname, size_t offset, size_t varsize, int flags )
{
	return new SendProp( varname, offset, varsize, 0, SendProxy_CString, flags );
}

template <class T>
void SendProxy_Vec( SendProp *prop, void *object, void *data, Datagram &dg )
{
	T vec = *(T *)data;
	vec.write_datagram_fixed( dg );
}

SendProp *SendPropVec3( const char *varname, size_t offset, size_t varsize, int flags )
{
	return new SendProp( varname, offset, varsize, 0, SendProxy_Vec<LVector3>, flags );
}

SendProp *SendPropVec4( const char *varname, size_t offset, size_t varsize, int flags )
{
	return new SendProp( varname, offset, varsize, 0, SendProxy_Vec<LVector4>, flags );
}

SendProp *SendPropVec2( const char *varname, size_t offset, size_t varsize, int flags )
{
	return new SendProp( varname, offset, varsize, 0, SendProxy_Vec<LVector2>, flags );
}
