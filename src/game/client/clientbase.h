#pragma once

#include <windowProperties.h>

#include "config_clientdll.h"
#include "hostbase.h"
#include "cl_netinterface.h"
#include "cl_entitymanager.h"
#include "shader_generator.h"
#include "game_postprocess.h"
#include "audio_3d_manager.h"
#include "ui/frame_rate_meter.h"
#include "cl_input.h"

class C_BasePlayer;

class ClientBase;
extern EXPORT_CLIENT_DLL ClientBase *cl;

/**
 * This is the client-side game framework.
 * It serves to initialize, contain, and manage all game/engine systems
 * related to the client.
 */
class EXPORT_CLIENT_DLL ClientBase : public HostBase
{
public:
	ClientBase();

	virtual bool startup();
	virtual void shutdown();

	virtual void mount_multifiles();
	virtual void add_game_systems();

	virtual void do_frame();

	virtual void run_cmd( const std::string &cmd );

	virtual void start_game( const std::string &mapname );

	INLINE CInput *get_input() const
	{
		return _input;
	}

	static INLINE ClientBase *ptr()
	{
		return cl;
	}

protected:
	virtual void load_cfg_files();

private:
	DECLARE_TASK_FUNC( cmd_task );

public:	
	PT( CFrameRateMeter ) _fps_meter;
	PT( CInput ) _input;

protected:
	PT( GenericAsyncTask ) _entities_task;
	PT( GenericAsyncTask ) _cmd_task;
};