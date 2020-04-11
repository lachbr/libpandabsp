#include "cl_entitymanager.h"
#include "c_baseentity.h"
#include "netmessages.h"
#include "clientbase.h"
#include "c_basecomponent.h"

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
		entid_t entnum = dgi.get_uint16();

		// Make sure this entity exists, if not, create it.
		bool new_ent = false;
		PT( C_BaseEntity ) ent = DCAST( C_BaseEntity, get_entity( entnum ) );
		if ( !ent )
		{
			ent = new C_BaseEntity;
			ent->init( entnum );
			ent->precache();
			insert_entity( ent );
			new_ent = true;
		}

		//if ( !ent )
		//{
		//	clents_cat.error()
		//		<< "No client definition for " << network_name << "\n";
		//	break;
		//}

		int num_comps = dgi.get_uint8();

		for ( int icomp = 0; icomp < num_comps; icomp++ )
		{
			componentid_t compid = dgi.get_uint16();

			PT( C_BaseComponent ) comp = nullptr;
			if ( new_ent )
			{
				C_BaseComponent *csingle = (C_BaseComponent *)get_component( compid );
				if ( csingle )
				{
					comp = DCAST( C_BaseComponent, csingle->make_new() );
					ent->add_component( comp );
					comp->initialize();
					comp->precache();
				}
			}
			else
			{
				comp = DCAST( C_BaseComponent, ent->get_component_by_id( compid ) );
			}

			int num_props = dgi.get_uint8();

			for ( int iprop = 0; iprop < num_props; iprop++ )
			{
				uint8_t prop_id = dgi.get_uint8();
				RecvProp *prop = (RecvProp *)( comp->get_network_class()
							       ->get_inherited_prop_by_id( prop_id ) );
				void *out = (unsigned char *)comp.p() + prop->get_offset();
				prop->get_proxy()( prop, comp, out, dgi );
			}
		}
		//int num_props = dgi.get_uint16();

		

		//clents_cat.debug()
		//	<< "Num props on " << network_name << ": " << num_props << std::endl;

		

		if ( new_ent )
			new_ents.push_back( ent );
		if ( num_comps > 0 )
			changed_ents.push_back( ent );
	}

	cl->set_tickcount( savetickcount );

	for ( C_BaseEntity *ent : new_ents )
		ent->spawn();
	for ( C_BaseEntity *ent : changed_ents )
		ent->post_data_update();
}

void ClientEntitySystem::update( double frametime )
{
	C_BaseComponent::interpolate_components();

	size_t count = edict.size();
	for ( size_t i = 0; i < count; i++ )
	{
		C_BaseEntity *ent = DCAST( C_BaseEntity, edict.get_data( i ) );
		ent->simulate( cl->get_frametime() );
	}
}