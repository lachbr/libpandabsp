#include "cl_entitymanager.h"
#include "c_entregistry.h"
#include "c_baseentity.h"
#include "netmessages.h"

ClientEntitySystem::ClientEntitySystem() :
	BaseEntitySystem()
{
	_local_player = nullptr;
	_local_player_id;
}

bool ClientEntitySystem::initialize()
{
	if ( !BaseClass::initialize() )
	{
		return false;
	}

	C_EntRegistry *reg = C_EntRegistry::ptr();
	reg->init_client_classes();

	return true;
}

bool ClientEntitySystem::handle_datagram( int msgtype, DatagramIterator &dgi )
{
	switch ( msgtype )
	{
	case NETMSG_DELETE_ENTITY:
		{
			entid_t entnum = dgi.get_uint32();
			remove_entity( entnum );
			return true;
		}
	case NETMSG_SNAPSHOT:
		{
			receive_snapshot( dgi );
			return true;
		}
	}

	// This wasn't one of the messages we care about. Tell the ClientNetSystem
	// to continue with the message.
	return false;
}

void ClientEntitySystem::receive_snapshot( DatagramIterator &dgi )
{
	pvector<C_BaseEntity *> new_ents;
	pvector<C_BaseEntity *> changed_ents;
	int num_ents = dgi.get_uint32();
	for ( int ient = 0; ient < num_ents; ient++ )
	{
		entid_t entnum = dgi.get_uint32();
		std::string network_name = dgi.get_string();
		int num_props = dgi.get_uint16();

		// Make sure this entity exists, if not, create it.
		bool new_ent = false;
		PT( C_BaseEntity ) ent = DCAST( C_BaseEntity, get_entity( entnum ) );
		if ( !ent )
		{
			ent = make_entity( network_name, entnum );
			new_ent = true;
		}

		nassertv( ent != nullptr );

		for ( int iprop = 0; iprop < num_props; iprop++ )
		{
			std::string prop_name = dgi.get_string();
			RecvProp *prop = ent->get_client_class()->
				get_recv_table().find_recv_prop( prop_name );
			nassertv( prop != nullptr );
			void *out = (unsigned char *)ent.p() + prop->get_offset();
			prop->get_proxy()( prop, ent, out, dgi );
		}

		if ( new_ent )
			new_ents.push_back( ent );
		if ( num_props > 0 )
			changed_ents.push_back( ent );
	}

	for ( C_BaseEntity *ent : new_ents )
		ent->spawn();
	for ( C_BaseEntity *ent : changed_ents )
		ent->post_data_update();
}

C_BaseEntity *ClientEntitySystem::make_entity( const std::string &network_name, entid_t entnum )
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