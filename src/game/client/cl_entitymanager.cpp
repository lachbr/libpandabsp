#include "cl_entitymanager.h"
#include "c_entregistry.h"

C_BaseEntity *ClientEntityManager::make_entity( const std::string &network_name, entid_t entnum )
{
	PT( C_BaseEntity ) ent = nullptr;

	C_EntRegistry *reg = C_EntRegistry::ptr();

	for ( auto itr = reg->_networkname_to_class.begin();
	      itr != reg->_networkname_to_class.end(); ++itr )
	{
		std::string nname = itr->first;
		C_BaseEntity *singleton = itr->second;

		if ( nname == network_name )
		{
			//c_client_cat.debug() << "Making client entity " << network_name << std::endl;
			ent = singleton->make_new();
			ent->init( entnum );
			ent->precache();
			insert_entity( ent );
			return ent;
		}
	}

	return ent;
}