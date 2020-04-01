#pragma once

#include <pandabase.h>
#include <pmap.h>
#include <pvector.h>

#include "config_serverdll.h"

class CBaseEntity;
class ServerClass;

class EXPORT_SERVER_DLL CEntRegistry
{
public:
	INLINE static CEntRegistry *ptr()
	{
		if ( !_ptr )
			_ptr = new CEntRegistry;
		return _ptr;
	}

	void link_networkname_to_class( const std::string &name, CBaseEntity *ent );
	void add_network_class( ServerClass *cls );

	void init_server_classes();

	pmap<std::string, CBaseEntity *> _networkname_to_class;
	pvector<ServerClass *> _network_class_list;

private:
	static CEntRegistry *_ptr;
};