#include "baseentitysystem.h"
#include "iclientdatagramhandler.h"

class C_BaseEntity;
class C_BasePlayer;

class ClientEntitySystem : public BaseEntitySystem, public IClientDatagramHandler
{
	DECLARE_CLASS( ClientEntitySystem, BaseEntitySystem )

public:
	ClientEntitySystem();

	virtual const char *get_name() const;
	virtual bool initialize();

	virtual bool handle_datagram( int msgtype, DatagramIterator &dgi );

	C_BaseEntity *make_entity( const std::string &network_name, entid_t entnum );

	void set_local_player_id( entid_t id );

private:
	void receive_snapshot( DatagramIterator &dgi );

private:
	C_BasePlayer *_local_player;
	entid_t _local_player_id;
};

INLINE void ClientEntitySystem::set_local_player_id( entid_t id )
{
	_local_player_id = id;
}

INLINE const char *ClientEntitySystem::get_name() const
{
	return "ClientEntitySystem";
}