#pragma once

#include "hostbase.h"
#include "sv_netinterface.h"
#include "sv_entitymanager.h"

class ServerBase;
extern EXPORT_SERVER_DLL ServerBase *sv;

/**
 * This is the server-side game framework.
 * It serves to initialize, contain, and manage all game/engine systems
 * related to the server.
 */
class EXPORT_SERVER_DLL ServerBase : public HostBase
{
public:
	ServerBase();

	virtual void run_cmd( const std::string &full_cmd, Client *cl = nullptr );

	virtual void mount_multifiles();
	virtual void end_tick();
	virtual void add_game_systems();

	INLINE const NodePath &get_root() const;

	static INLINE ServerBase *ptr()
	{
		return sv;
	}

protected:
	virtual void load_cfg_files();

	NodePath _root;
};

INLINE const NodePath &ServerBase::get_root() const
{
	return _root;
}