#ifndef BASEGAME_H_
#define BASEGAME_H_

#include "config_game_shared.h"
#include "bsploader.h"
#include "tasks.h"
#include "igamesystem.h"

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
#include <simpleHashMap.h>
#include <pvector.h>

class EXPORT_GAME_SHARED HostBase : public ReferenceCount
{
public:
	HostBase();

	void add_game_system( IGameSystem *system, int sort = 0, bool per_frame = false );
	void remove_game_system( IGameSystem *system );
	void remove_game_system( TypeHandle handle );

	template <class Type>
	bool get_game_system( Type *&system ) const;
	template <class Type>
	bool get_game_system( PT( Type ) &system ) const;

	IGameSystem *get_game_system( TypeHandle handle ) const;

	void load_cfg_file( const Filename &cfgpath );
	void load_cfg_data( const std::string &data );

	void mount_multifile( const Filename &mfpath );

	virtual bool startup();
	virtual void shutdown();

	virtual void do_frame();
	void run();

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

	INLINE VirtualFileSystem *get_vfs() const
	{
		return _vfs;
	}

protected:
	virtual void add_game_systems() = 0;

	virtual void load_cfg_files();
	virtual void mount_multifiles();

public:
	ClockObject *_global_clock;
	Loader *_loader;
	EventHandler *_event_mgr;
	VirtualFileSystem *_vfs;

protected:
	class GameSystemEntry
	{
	public:
		int sort;
		PT( IGameSystem ) system;
		bool per_frame;

		class Sorter
		{
			constexpr bool operator()( const GameSystemEntry &left, const GameSystemEntry &right ) const
			{
				return left.sort < right.sort;
			}
		};
	};

	SimpleHashMap<TypeHandle, GameSystemEntry> _game_systems;
	ordered_vector<GameSystemEntry, GameSystemEntry::Sorter> _game_systems_sorted;

	bool _quit;

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

INLINE void HostBase::remove_game_system( IGameSystem *sys )
{
	remove_game_system( sys->get_type() );
}

INLINE void HostBase::remove_game_system( TypeHandle type )
{
	int isys = _game_systems.find( type );
	if ( isys != -1 )
	{
		GameSystemEntry entry = _game_systems.get_data( isys );
		entry.system->shutdown();
		_game_systems_sorted.erase( entry );
		_game_systems.remove_element( isys );
	}
}

template <class Type>
INLINE bool HostBase::get_game_system( Type *&system ) const
{
	system = (Type *)get_game_system( Type::get_class_type() );
	return system != nullptr;
}

template <class Type>
INLINE bool HostBase::get_game_system( PT( Type ) &system ) const
{
	system = (Type *)get_game_system( Type::get_class_type() );
	return system != nullptr;
}

INLINE IGameSystem *HostBase::get_game_system( TypeHandle handle ) const
{
	int isys = _game_systems.find( handle );
	if ( isys != -1 )
	{
		return _game_systems.get_data( isys ).system;
	}

	return nullptr;
}

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