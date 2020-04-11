#pragma once

#include "baseentitysystem.h"
#include "iclientdatagramhandler.h"
//#include "c_basecomponent.h"

class C_BaseEntity;
class BaseComponentShared;
//class C_BaseComponent;
//class C_BasePlayer;

class ClientEntitySystem : public BaseEntitySystem, public IClientDatagramHandler
{
	DECLARE_CLASS( ClientEntitySystem, BaseEntitySystem )

public:
	ClientEntitySystem();

	virtual const char *get_name() const;
	virtual void update( double frametime );
	virtual bool initialize();

	virtual bool handle_datagram( int msgtype, DatagramIterator &dgi );

	//C_BaseComponent *make_entity( const std::string &network_name, entid_t entnum );

	void set_local_player_id( entid_t id );
	void set_local_player( C_BaseEntity *plyr );
	C_BaseEntity *get_local_player() const;
	entid_t get_local_player_id() const;

	BaseComponentShared *get_component_from_networkname( const std::string &name ) const;
	void link_networkname_to_component( const std::string &name, BaseComponentShared *ent_singleton );

	static ClientEntitySystem *ptr();

private:
	void receive_snapshot( DatagramIterator &dgi );

private:
	C_BaseEntity *_local_player;
	entid_t _local_player_id;
	SimpleHashMap<std::string, BaseComponentShared *, string_hash> _networkname_to_component;
};

INLINE ClientEntitySystem *ClientEntitySystem::ptr()
{
	static ClientEntitySystem *global_ptr = new ClientEntitySystem;
	return global_ptr;
}

INLINE void ClientEntitySystem::link_networkname_to_component( const std::string &name, BaseComponentShared *ent_singleton )
{
	_networkname_to_component[name] = ent_singleton;
}

INLINE BaseComponentShared *ClientEntitySystem::get_component_from_networkname( const std::string &name ) const
{
	int isingle = _networkname_to_component.find( name );
	if ( isingle != -1 )
	{
		return _networkname_to_component.get_data( isingle );
	}
	return nullptr;
}

INLINE void ClientEntitySystem::set_local_player_id( entid_t id )
{
	_local_player_id = id;
}

INLINE void ClientEntitySystem::set_local_player( C_BaseEntity *plyr )
{
	_local_player = plyr;
}

inline C_BaseEntity *ClientEntitySystem::get_local_player() const
{
	return _local_player;
}

inline entid_t ClientEntitySystem::get_local_player_id() const
{
	return _local_player_id;
}

INLINE const char *ClientEntitySystem::get_name() const
{
	return "ClientEntitySystem";
}

extern ClientEntitySystem *clents;