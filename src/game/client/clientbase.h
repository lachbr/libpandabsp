#pragma once

#include <windowProperties.h>

#include "config_clientdll.h"
#include "hostbase.h"
#include "cl_netinterface.h"
#include "cl_entitymanager.h"
#include "shader_generator.h"
#include "game_postprocess.h"
#include "audio_3d_manager.h"
#include "keymappings.h"
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

	virtual void do_net_tick();
	virtual void do_game_tick();
	virtual void do_frame();

	virtual void load_bsp_level( const Filename &path, bool is_transition = false );
	virtual void cleanup_bsp_level();

	virtual void run_cmd( const std::string &cmd );

	virtual void start_game( const std::string &mapname );

	INLINE LVector2i get_size() const
	{
		if ( !_graphics_window )
			return LVector2i::zero();

		return LVector2i( _graphics_window->get_x_size(),
				  _graphics_window->get_y_size() );
	}

	INLINE float get_aspect_ratio() const
	{
		if ( !_graphics_window )
			return 1.0f;

		return (float)_graphics_window->get_sbs_left_x_size() / (float)_graphics_window->get_sbs_left_y_size();
	}

	INLINE GraphicsEngine *get_graphics_engine() const
	{
		return _graphics_engine;
	}

	INLINE GraphicsStateGuardian *get_gsg() const
	{
		return _gsg;
	}

	INLINE GraphicsPipe *get_graphics_pipe() const
	{
		return _graphics_pipe;
	}

	INLINE GraphicsPipeSelection *get_graphics_pipe_selection() const
	{
		return _graphics_pipe_selection;
	}

	INLINE GraphicsWindow *get_graphics_window() const
	{
		return _graphics_window;
	}

	INLINE ClientNetInterface *get_net_interface() const
	{
		return _client;
	}

	INLINE void set_local_player( C_BasePlayer *plyr )
	{
		_local_player = plyr;
	}

	INLINE C_BasePlayer *get_local_player() const
	{
		return _local_player;
	}

	INLINE void set_local_player_entnum( entid_t entnum )
	{
		_local_player_entnum = entnum;
	}

	INLINE entid_t get_local_player_entnum() const
	{
		return _local_player_entnum;
	}

	void adjust_window_aspect_ratio( float ratio );

	INLINE CKeyMappings *get_key_mappings() const
	{
		return _key_mappings;
	}

	INLINE CInput *get_input() const
	{
		return _input;
	}

	ClientEntityManager *get_entity_mgr();

	static INLINE ClientBase *ptr()
	{
		return cl;
	}

protected:
	virtual void setup_camera();
	virtual void make_bsp();
	virtual void setup_bsp();
	virtual void setup_scene();
	virtual void setup_rendering();
	virtual void setup_shaders();
	virtual void setup_dgraph();
	virtual void setup_audio();
	virtual void setup_tasks();
	virtual void setup_postprocess();
	virtual void setup_input();

	virtual void load_cfg_files();

	void window_event();
	static void handle_window_event( const Event *e, void *data );

private:
	DECLARE_TASK_FUNC( render_task );
	DECLARE_TASK_FUNC( data_task );
	DECLARE_TASK_FUNC( audio_task );
	DECLARE_TASK_FUNC( entities_task );
	DECLARE_TASK_FUNC( client_task );
	DECLARE_TASK_FUNC( cmd_task );

public:
	GraphicsEngine *_graphics_engine;
	PT( GraphicsStateGuardian ) _gsg;
	PT( GraphicsPipe ) _graphics_pipe;
	GraphicsPipeSelection *_graphics_pipe_selection;
	PT( GraphicsWindow ) _graphics_window;
	PT( BSPShaderGenerator ) _shader_generator;
	PT( Audio3DManager ) _audio3d;
	PT( GamePostProcess ) _post_process;
	DataGraphTraverser _dgtrav;
	PT( AudioManager ) _sfx_manager;
	PT( AudioManager ) _music_manager;
	InputDeviceManager *_input_mgr;
	PT( ClientNetInterface ) _client;
	ClientEntityManager _entmgr;
	PT( PerspectiveLens ) _cam_lens;
	PT( CFrameRateMeter ) _fps_meter;
	PT( CKeyMappings ) _key_mappings;
	PT( CInput ) _input;

	entid_t _local_player_entnum;
	C_BasePlayer *_local_player;

	NodePath _camera;
	NodePath _cam;
	NodePath _aspect2d;
	NodePath _render2d;
	NodePath _pixel2d;

	float _a2d_top;
	float _a2d_bottom;
	float _a2d_left;
	float _a2d_right;

	NodePath _a2d_background;
	NodePath _a2d_top_center;
	NodePath _a2d_top_center_ns;
	NodePath _a2d_bottom_center;
	NodePath _a2d_bottom_center_ns;
	NodePath _a2d_left_center;
	NodePath _a2d_left_center_ns;
	NodePath _a2d_right_center;
	NodePath _a2d_right_center_ns;

	NodePath _a2d_top_left;
	NodePath _a2d_top_left_ns;
	NodePath _a2d_top_right;
	NodePath _a2d_top_right_ns;
	NodePath _a2d_bottom_left;
	NodePath _a2d_bottom_left_ns;
	NodePath _a2d_bottom_right;
	NodePath _a2d_bottom_right_ns;

	NodePath _dataroot;
	NodePathCollection _mouse_and_keyboard;
	NodePathCollection _mouse_watcher;
	NodePathCollection _kb_button_thrower;

	WindowProperties _prev_window_props;

	bool _window_foreground;
	bool _window_minimized;
	float _old_aspect_ratio;

protected:
	PT( GenericAsyncTask ) _render_task;
	PT( GenericAsyncTask ) _data_task;
	PT( GenericAsyncTask ) _audio_task;
	PT( GenericAsyncTask ) _entities_task;
	PT( GenericAsyncTask ) _client_task;
	PT( GenericAsyncTask ) _cmd_task;
};

INLINE ClientEntityManager *ClientBase::get_entity_mgr()
{
	return &_entmgr;
}