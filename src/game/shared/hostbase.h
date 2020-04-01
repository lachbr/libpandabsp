#ifndef BASEGAME_H_
#define BASEGAME_H_

#include "config_game_shared.h"
#include "bsploader.h"
#include "tasks.h"

#include <filename.h>
#include <asyncTaskManager.h>
#include <genericAsyncTask.h>
#include <loader.h>
#include <referenceCount.h>
#include <graphicsEngine.h>
#include <graphicsStateGuardian.h>
#include <graphicsPipe.h>
#include <graphicsPipeSelection.h>
#include <graphicsWindow.h>
#include <nodePath.h>
#include <bulletWorld.h>
#include <dataGraphTraverser.h>
#include <audioManager.h>
#include <cIntervalManager.h>
#include <inputDeviceManager.h>
#include <eventHandler.h>
#include <virtualFileSystem.h>

class EXPORT_GAME_SHARED HostBase : public ReferenceCount
{
public:
	HostBase();

	void load_cfg_file( const Filename &cfgpath );
	void load_cfg_data( const std::string &data );

	virtual Filename get_map_filename( const std::string &mapname );

	void mount_multifile( const Filename &mfpath );

	virtual void cleanup_bsp_level();
	virtual void load_bsp_level( const Filename &path, bool is_transition = false );

	virtual bool startup();
	virtual void shutdown();

	virtual void do_frame();
	void run();

	virtual void do_net_tick() = 0;
	virtual void do_game_tick() = 0;

	void set_tick_rate( int tickrate );

	int get_network_base( int tick, int entity ) const;

	int get_tickcount() const;
	double get_curtime() const;
	double get_frametime() const;
	bool is_paused() const;

	ClockObject *get_clock() const;

	INLINE Loader *get_loader() const
	{
		return _loader;
	}

	INLINE AsyncTaskManager *get_task_manager() const
	{
		return _task_manager;
	}

	INLINE BSPLoader *get_bsp_loader() const
	{
		return _bsp_loader;
	}

	INLINE VirtualFileSystem *get_vfs() const
	{
		return _vfs;
	}

protected:
	virtual void make_bsp() = 0;
	virtual void setup_rendering();
	virtual void setup_scene();
	virtual void setup_camera();
	virtual void setup_dgraph();
	virtual void setup_physics();
	virtual void setup_audio();
	virtual void setup_tasks();
	virtual void setup_shaders();
	virtual void setup_bsp();
	virtual void load_cfg_files();
	virtual void mount_multifiles();

private:
	DECLARE_TASK_FUNC( physics_task );
	DECLARE_TASK_FUNC( garbage_collect_task );
	DECLARE_TASK_FUNC( ival_task );
	DECLARE_TASK_FUNC( reset_prev_transform_task );

public:
	ClockObject *_global_clock;
	AsyncTaskManager *_task_manager;
	Loader *_loader;
	BSPLoader *_bsp_loader;
	NodePath _bsp_level;
	EventHandler *_event_mgr;
	CIntervalManager *_ival_mgr;
	VirtualFileSystem *_vfs;
	PT( BulletWorld ) _physics_world;

	std::string _map;
	
	NodePath _render;
	NodePath _hidden;

protected:
	bool _quit;

	PT( GenericAsyncTask ) _garbage_collect_task;
	PT( GenericAsyncTask ) _ival_task;
	PT( GenericAsyncTask ) _physics_task;
	PT( GenericAsyncTask ) _reset_prev_transform_task;

	int _tickcount;
	int _frameticks;
	int _currentframetick;
	int _simticksthisframe;
	double _curtime;
	double _oldcurtime;
	double _realframetime;
	double _frametime;
	int _framecount;
	double _remainder;
	int _tickrate;
	double _intervalpertick;
	double _interpolationamount;
	int _timestamp_networking_base;
	int _timestamp_randomize_window;
	bool _paused;
};

INLINE ClockObject *HostBase::get_clock() const
{
	return _global_clock;
}

INLINE double HostBase::get_curtime() const
{
	return _curtime;
}

INLINE double HostBase::get_frametime() const
{
	return _frametime;
}

INLINE int HostBase::get_tickcount() const
{
	return _tickcount;
}

INLINE bool HostBase::is_paused() const
{
	return _paused;
}

INLINE void HostBase::set_tick_rate( int rate )
{
	_tickrate = rate;
	_intervalpertick = 1.0 / rate;
}

INLINE int HostBase::get_network_base( int tick, int entity ) const
{
	int entity_mod = entity % _timestamp_randomize_window;
	int base_tick = _timestamp_networking_base *
		(int)( ( tick - entity_mod ) / _timestamp_networking_base );
	return base_tick;
}

#endif // BASEGAME_H_