#pragma once

#include "hostbase.h"
#include "sv_netinterface.h"

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

	ServerNetInterface *get_net_interface() const;

	virtual void run_cmd( const std::string &full_cmd, Client *cl = nullptr );

	virtual void load_bsp_level( const Filename &path, bool is_transition = false );

	virtual bool startup();
	virtual void make_bsp();
	virtual void setup_bsp();
	virtual void do_net_tick();
	virtual void do_game_tick();
	virtual void do_frame();
	virtual void run_entities( bool simulating );

	void send_tick();

	int time_to_ticks( double dt );
	double ticks_to_time( double dt );

	static INLINE ServerBase *ptr()
	{
		return sv;
	}

protected:
	virtual void setup_tasks();
	virtual void load_cfg_files();

public:
	PT( ServerNetInterface ) _server;
};

INLINE double ServerBase::ticks_to_time( double dt )
{
	return ( _intervalpertick * (float)( dt ) );
}

INLINE int ServerBase::time_to_ticks( double dt )
{
	return ( (int)( 0.5f + (float)( dt ) / _intervalpertick ) );
}

INLINE ServerNetInterface *ServerBase::get_net_interface() const
{
	return _server;
}