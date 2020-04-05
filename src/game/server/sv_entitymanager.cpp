#include "sv_entitymanager.h"
#include "baseentity.h"
#include "netmessages.h"
#include "serverbase.h"
#include "sv_netinterface.h"
#include "baseplayer.h"

#define SPEW_NETWORK_CLASSES 0

ServerEntitySystem *svents = nullptr;

IMPLEMENT_CLASS( ServerEntitySystem )

ServerEntitySystem::ServerEntitySystem() :
	_entnum_alloc( ENTID_MIN + 1, ENTID_MAX )
{
	svents = this;
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
		NetworkClass *nclass = singleton->get_network_class();
		std::cout << "\t" << nclass->get_name() << std::endl;
		size_t pcount = nclass->get_num_inherited_props();
		for ( size_t j = 0; j < pcount; j++ )
		{
			NetworkProp *prop = nclass->get_inherited_prop( j );
			std::cout << "\t\t- " << prop->get_name() << std::endl;
		}
	}
#endif

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

void ServerEntitySystem::build_snapshot( Datagram &dg, bool full_snapshot )
{
	Datagram snapshot_body;

	int num_changed = 0;
	size_t size = edict.size();
	for ( size_t i = 0; i < size; i++ )
	{
		CBaseEntity *ent = DCAST( CBaseEntity, edict.get_data( i ) );
		NetworkClass *cls = ent->get_network_class();

		int num_props;
		if ( full_snapshot || ent->is_entity_fully_changed() )
			num_props = cls->get_num_inherited_props();
		else
			num_props = ent->get_num_changed_offsets();

		if ( num_props == 0 )
			continue;

		snapshot_body.add_uint32( ent->get_entnum() );
		snapshot_body.add_string( cls->get_name() );

		snapshot_body.add_uint16( num_props );

		int props_added = 0;
		int total_props = cls->get_num_inherited_props();
		for ( int j = 0; j < total_props; j++ )
		{
			
			SendProp *prop = (SendProp *)cls->get_inherited_prop( j );
			if ( full_snapshot || ent->is_entity_fully_changed() || ent->is_property_changed( prop ) )
			{

				snapshot_body.add_string( prop->get_name() );
				//std::cout << "propname: " << prop.get_prop_name() << std::endl;
				void *data = (unsigned char *)ent + prop->get_offset();
				prop->get_proxy()( prop, ent, data, snapshot_body );
				props_added++;
			}
		}

		if ( num_props != props_added )
		{
			nassert_raise( "num_props != props_added!!!!" );
			cout << "num_props = " << num_props << ", props_added = " << props_added << std::endl;
		}

		if ( !full_snapshot )
			ent->reset_changed_offsets();

		num_changed++;
	}

	dg.add_uint16( NETMSG_SNAPSHOT );
	dg.add_uint32( sv->get_tickcount() );
	dg.add_uint32( num_changed );
	// Add the actual entity snapshots
	dg.append_data( snapshot_body.get_data(), snapshot_body.get_length() );
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
			ent->run_thinks();
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
				cl->get_player()->run_thinks();
		}
	}
}

void ServerEntitySystem::update( double frametime )
{
	run_entities( !sv->is_paused() );
}