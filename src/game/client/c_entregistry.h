#pragma once

#include <pandabase.h>
#include <pmap.h>
#include <pvector.h>

#include "config_clientdll.h"

class C_BaseEntity;
class ClientClass;

class EXPORT_CLIENT_DLL C_EntRegistry
{
public:
	INLINE static C_EntRegistry *ptr()
	{
		if ( !_ptr )
			_ptr = new C_EntRegistry;
		return _ptr;
	}

	void link_networkname_to_class( const std::string &name, C_BaseEntity *ent );
	void add_network_class( ClientClass *cls );

	void init_client_classes();

	pmap<std::string, C_BaseEntity *> _networkname_to_class;
	pvector<ClientClass *> _network_class_list;

private:
	static C_EntRegistry *_ptr;
};