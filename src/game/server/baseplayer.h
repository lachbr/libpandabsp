#pragma once

#include "baseentity.h"
//#include "baseplayer_shared.h"

NotifyCategoryDeclNoExport( baseplayer )

class Client;
//class CPlayerComponent;

class EXPORT_SERVER_DLL CBasePlayer : public CBaseEntity
{
	DECLARE_CLASS( CBasePlayer, CBaseEntity )

public:
	CBasePlayer();

	virtual void add_components();
	virtual void init( entid_t entnum );

	//virtual void setup_controller();
	//virtual void spawn();

	INLINE void set_client( Client *cl )
	{
		_client = cl;
	}

	INLINE Client *get_client() const
	{
		return _client;
	}

private:
	//NetworkVec3( _view_offset );
	Client *_client;

	//CPlayerComponent *_player_component;
};

