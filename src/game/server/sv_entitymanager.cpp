#include "sv_entitymanager.h"
#include "baseentity.h"
#include "netmessages.h"
#include "serverbase.h"
#include "sv_netinterface.h"
#include "baseplayer.h"
#include "basecomponent.h"

#define SPEW_NETWORK_CLASSES 0

ServerEntitySystem *svents = nullptr;

IMPLEMENT_CLASS( ServerEntitySystem )

ServerEntitySystem::ServerEntitySystem() :
	_entnum_alloc( ENTID_MIN + 1, ENTID_MAX )
{
}

bool ServerEntitySystem::initialize()
{
	if ( !BaseClass::initialize() )
		return false;

#if SPEW_NETWORK_CLASSES
	std::cout << "Server network classes: " << std::endl;
	size_t count = _entity_to_class.size();
	for ( size_t i = 0; i < count; i++ )
	{
		CBaseEntity *singleton = _entity_to_class.get_data( i );
		DataTable *nclass = singleton->get_network_class();
		std::cout << "\t" << nclass->get_name() << std::endl;
		size_t pcount = nclass->get_num_inherited_props();
		for ( size_t j = 0; j < pcount; j++ )
		{
			NetworkProp *prop = nclass->get_inherited_prop( j );
			std::cout << "\t\t- " << prop->get_name() << std::endl;
		}
	}
#endif

	svents = this;
	return true;
}

bool ServerEntitySystem::handle_datagram( Client *client, int msgtype, DatagramIterator &dgi )
{
	switch ( msgtype )
	{
	case NETMSG_ENTITY_MSG:
		{
			// Received an entity message from one of the clients.

			entid_t entnum = dgi.get_uint32();
			int entmsgtype = dgi.get_uint8();
			
			int ient = edict.find( entnum );
			if ( ient != -1 )
			{
				CBaseEntity *ent = DCAST( CBaseEntity, edict.get_data( ient ) );
				ent->receive_entity_message( entmsgtype, client->get_client_id(), dgi );
			}

			return true;
		}
	}

	return false;
}

CBaseEntity *ServerEntitySystem::make_entity_by_name( const std::string &name, bool spawn,
						       bool bexplicit_entnum, entid_t explicit_entnum )
{
	int isingle = _entity_to_class.find( name );
	if ( isingle == -1 )
	{
		return nullptr;
	}

	CBaseEntity *singleton = _entity_to_class.get_data( isingle );
	PT( CBaseEntityShared ) ent_shared = singleton->make_new();
	CBaseEntity *ent = (CBaseEntity *)ent_shared.p();
	if ( bexplicit_entnum )
		ent->init( explicit_entnum );
	else
		ent->init( _entnum_alloc.allocate() );
	ent->precache();

	if ( spawn )
		ent->spawn();

	insert_entity( ent );

	return ent;
}

void ServerEntitySystem::reset_all_changed_props()
{
	size_t count = edict.size();
	for ( size_t i = 0; i < count; i++ )
	{
		CBaseEntity *ent = DCAST( CBaseEntity, edict.get_data( i ) );
		ent->reset_changed_offsets();
	}
}

inline static int serialize_component( Datagram &comp_dg, CBaseEntity *ent, uint32_t client_id,
				       CBaseComponent *comp, bool full_snapshot )
{
	NetworkClass *cls = comp->get_network_class();

	if ( cls->has_flags( SENDFLAGS_OWNRECV ) && client_id != ent->get_owner_client() )
	{
		// This component should only be seen by owners.
		return 0;
	}

	int num_props;
	if ( full_snapshot || comp->is_fully_changed() )
		num_props = cls->get_num_inherited_props();
	else
		num_props = comp->get_num_changed_offsets();

	// If component has no props, build nothing
	if ( num_props == 0 )
		return 0;

	Datagram props_dg;

	int props_added = 0;

	int total_props = cls->get_num_inherited_props();
	for ( int j = 0; j < total_props; j++ )
	{
		SendProp *prop = (SendProp *)cls->get_inherited_prop( j );
		if ( full_snapshot || comp->is_fully_changed() || comp->is_property_changed( prop ) )
		{
			props_dg.add_uint8( prop->get_id() );
			//std::cout << "propname: " << prop.get_prop_name() << std::endl;
			void *data = (unsigned char *)comp + prop->get_offset();
			prop->get_proxy()( prop, comp, data, props_dg );
			props_added++;
		}
	}

	// If nothing changed, build nothing
	if ( props_added == 0 )
		return 0;

	comp_dg.add_uint16( comp->get_type_id() );
	comp_dg.add_uint8( props_added );
	comp_dg.append_data( props_dg.get_data(), props_dg.get_length() );

	return props_added;
}

bool ServerEntitySystem::build_snapshot( Datagram &dg, uint32_t client_id, bool full_snapshot )
{
	Datagram snapshot_body;

	int num_changed = 0;
	size_t size = edict.size();
	for ( size_t i = 0; i < size; i++ )
	{
		CBaseEntity *ent = DCAST( CBaseEntity, edict.get_data( i ) );

		Datagram comp_body;

		uint8_t comps_changed = 0;
		uint8_t comp_count = ent->get_num_components();
		for ( uint8_t c = 0; c < comp_count; c++ )
		{
			CBaseComponent *comp = DCAST( CBaseComponent, ent->get_component( c ) );

			if ( serialize_component( comp_body, ent, client_id, comp, full_snapshot ) > 0 )
			{
				comps_changed++;
			}
		}

		if ( comps_changed > 0 )
		{
			snapshot_body.add_uint16( ent->get_entnum() );
			snapshot_body.add_uint8( comps_changed );
			snapshot_body.append_data( comp_body.get_data(), comp_body.get_length() );
			num_changed++;
		}
	}

	// No need to send a snapshot.
	if ( num_changed == 0 )
		return false;

	dg.add_uint16( NETMSG_SNAPSHOT );
	dg.add_uint32( sv->get_tickcount() );
	dg.add_uint32( num_changed );
	// Add the actual entity data
	dg.append_data( snapshot_body.get_data(), snapshot_body.get_length() );

	return true;
}

void ServerEntitySystem::remove_entity( entid_t entnum )
{
	BaseClass::remove_entity( entnum );
	_entnum_alloc.free( entnum );

	Datagram dg = BeginMessage( NETMSG_DELETE_ENTITY );
	dg.add_uint32( entnum );
	svnet->broadcast_datagram( dg, true );
}

void ServerEntitySystem::run_entities( bool simulating )
{
	if ( simulating )
	{
		size_t ents = edict.size();
		for ( size_t i = 0; i < ents; i++ )
		{
			CBaseEntity *ent = DCAST( CBaseEntity, edict.get_data( i ) );
			ent->simulate( sv->get_frametime() );
		}
	}
	else
	{
		// Only simulate players
		ServerNetInterface::clientmap_t *clients = svnet->get_clients();
		for ( auto itr : *clients )
		{
			Client *cl = itr.second;
			if ( cl->get_player() )
				cl->get_player()->simulate( sv->get_frametime() );
		}
	}
}

void ServerEntitySystem::update( double frametime )
{
	run_entities( !sv->is_paused() );
}