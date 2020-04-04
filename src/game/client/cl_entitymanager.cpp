#include "cl_entitymanager.h"
#include "c_entregistry.h"
#include "c_baseentity.h"
#include "netmessages.h"

NotifyCategoryDeclNoExport( clents )
NotifyCategoryDef( clents, "" )

ClientEntitySystem *clents = nullptr;

IMPLEMENT_CLASS( ClientEntitySystem )

ClientEntitySystem::ClientEntitySystem() :
	BaseEntitySystem()
{
	clents = this;
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
	case NETMSG_ENTITY_MSG:
		{
			// Received an entity message from the server.

			entid_t entnum = dgi.get_uint32();
			int entmsgtype = dgi.get_uint8();

			int ient = edict.find( entnum );
			if ( ient != -1 )
			{
				C_BaseEntity *ent = DCAST( C_BaseEntity, edict.get_data( ient ) );
				ent->receive_entity_message( entmsgtype, dgi );
			}

			return true;
		}
	}

	// This wasn't one of the messages we care about. Tell the ClientNetSystem
	// to continue with the message.
	return false;
}

void ClientEntitySystem::receive_snapshot( DatagramIterator &dgi )
{
	
	int tickcount = dgi.get_uint32();
	int savetickcount = cl->get_tickcount();
	// Be on the same tick as this snapshot.
	cl->set_tickcount( tickcount );
	int num_ents = dgi.get_uint32();
	pvector<C_BaseEntity *> new_ents;
	new_ents.reserve( num_ents );
	pvector<C_BaseEntity *> changed_ents;
	changed_ents.reserve( num_ents );
	for ( int ient = 0; ient < num_ents; ient++ )
	{
		entid_t entnum = dgi.get_uint32();
		//std::cout << "entnum " << entnum << std::endl;
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

		if ( !ent )
		{
			clents_cat.error()
				<< "No client definition for " << network_name << "\n";
			break;
		}

		clents_cat.debug()
			<< "Num props on " << network_name << ": " << num_props << std::endl;

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

	cl->set_tickcount( savetickcount );

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

void ClientEntitySystem::update( double frametime )
{
	C_BaseEntity::interpolate_entities();

	size_t count = edict.size();
	for ( size_t i = 0; i < count; i++ )
	{
		C_BaseEntity *ent = DCAST( C_BaseEntity, edict.get_data( i ) );
		ent->run_thinks();
	}
}