#include "dt_recv.h"

#include <aa_luse.h>

void RecvProxy_Int8( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	( *(int8_t *)out ) = dgi.get_int8();
}

void RecvProxy_Int16( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	( *(int16_t *)out ) = dgi.get_int16();
}

void RecvProxy_Int32( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	( *(int32_t *)out ) = dgi.get_int32();
}

void RecvProxy_Int64( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	( *(int64_t *)out ) = dgi.get_int64();
}

void RecvProxy_Uint8( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	( *(uint8_t *)out ) = dgi.get_uint8();
}

void RecvProxy_Uint16( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	( *(uint16_t *)out ) = dgi.get_uint16();
}

void RecvProxy_Uint32( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	( *(uint32_t *)out ) = dgi.get_uint32();
}

void RecvProxy_Uint64( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	( *(uint64_t *)out ) = dgi.get_uint64();
}

void RecvProxy_Float32( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	( *(PN_float32 *)out ) = dgi.get_float32();
}

void RecvProxy_Float64( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	( *(PN_float64 *)out ) = dgi.get_float64();
}

void RecvProxy_String( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	( *( std::string * )out ) = dgi.get_string();
}

void RecvProxy_CString( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	std::string str = dgi.get_string();
	char *szout = (char *)out;
	if ( prop->get_varsize() <= 0 )
		return;

	for ( int i = 0; i < prop->get_varsize(); i++ )
	{
		szout[i] = str[i];
		if ( szout[i] == 0 )
			break;
	}

	szout[prop->get_varsize() - 1] = 0;
}

RecvProp RecvPropNull()
{
	return RecvProp( "__null__", 0, 0, nullptr, 0 );
}

RecvProp RecvPropInt( const char *varname, size_t offset, size_t varsize, RecvProxyFn proxy, int bits, int flags )
{
	if ( !proxy )
	{
		switch ( bits )
		{
		case 8:
			proxy = RecvProxy_Int8;
			break;
		case 16:
			proxy = RecvProxy_Int16;
			break;
		case 32:
			proxy = RecvProxy_Int32;
			break;
		case 64:
			proxy = RecvProxy_Int64;
			break;
		default:
			proxy = RecvProxy_Int32;
		}
	}
	
	return RecvProp( varname, offset, varsize, proxy, flags );
}

RecvProp RecvPropUint( const char *varname, size_t offset, size_t varsize, RecvProxyFn proxy, int bits, int flags )
{
	if ( !proxy )
	{
		switch ( bits )
		{
		case 8:
			proxy = RecvProxy_Uint8;
			break;
		case 16:
			proxy = RecvProxy_Uint16;
			break;
		case 32:
			proxy = RecvProxy_Uint32;
			break;
		case 64:
			proxy = RecvProxy_Uint64;
			break;
		default:
			proxy = RecvProxy_Uint32;
		}
	}
	
	return RecvProp( varname, offset, varsize, proxy, flags );
}

RecvProp RecvPropFloat( const char *varname, size_t offset, size_t varsize, RecvProxyFn proxy, int bits, int flags )
{
	if ( !proxy )
	{
		switch ( bits )
		{
		case 32:
			proxy = RecvProxy_Float32;
			break;
		case 64:
			proxy = RecvProxy_Float64;
			break;
		default:
			proxy = RecvProxy_Float32;
		}
	}
	
	return RecvProp( varname, offset, varsize, proxy, flags );
}

RecvProp RecvPropString( const char *varname, size_t offset, size_t varsize, RecvProxyFn proxy, int flags )
{
	if ( !proxy )
		proxy = RecvProxy_String;
	return RecvProp( varname, offset, varsize, proxy, flags );
}

RecvProp RecvPropCString( const char *varname, size_t offset, size_t varsize, RecvProxyFn proxy, int flags )
{
	if ( !proxy )
		proxy = RecvProxy_CString;
	return RecvProp( varname, offset, varsize, proxy, flags );
}

template <class T>
void RecvProxy_Vec( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	T vec;
	vec.read_datagram_fixed( dgi );

	( *(T *)out ) = vec;
}

RecvProp RecvPropVec4( const char *varname, size_t offset, size_t varsize, RecvProxyFn proxy, int flags )
{
	if ( !proxy )
		proxy = RecvProxy_Vec<LVector4>;
	return RecvProp( varname, offset, varsize, proxy, flags );
}

RecvProp RecvPropVec3( const char *varname, size_t offset, size_t varsize, RecvProxyFn proxy, int flags )
{
	if ( !proxy )
		proxy = RecvProxy_Vec<LVector3>;
	return RecvProp( varname, offset, varsize, proxy, flags );
}

RecvProp RecvPropVec2( const char *varname, size_t offset, size_t varsize, RecvProxyFn proxy, int flags )
{
	if ( !proxy )
		proxy = RecvProxy_Vec<LVector2>;
	return RecvProp( varname, offset, varsize, proxy, flags );
}

RecvProp *RecvTable::find_recv_prop( const std::string &name )
{
	for ( size_t i = 0; i < _props.size(); i++ )
	{
		RecvProp *prop = &_props[i];
		if ( !strcmp( prop->get_prop_name(), name.c_str() ) )
		{
			return prop;
		}
	}

	std::cout << "Could find recv prop " << name << std::endl;
	return nullptr;
}