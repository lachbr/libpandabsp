#include "hostbase.h"
#include "physics_utils.h"

#include "bsp_render.h"

#include <graphicsPipeSelection.h>
#include <frameBufferProperties.h>
#include <perspectiveLens.h>
#include <camera.h>
#include <displayRegion.h>
#include <configVariableInt.h>
#include <configVariableDouble.h>
#include <modelRoot.h>
#include <rescaleNormalAttrib.h>
#include <pgTop.h>
#include <mouseWatcher.h>
#include <mouseAndKeyboard.h>
#include <buttonThrower.h>
#include <load_prc_file.h>
#include <pStatClient.h>
#include <notifyCategoryProxy.h>

NotifyCategoryDeclNoExport( hostbase )
NotifyCategoryDef( hostbase, "" )

HostBase::HostBase() :
	_loader( Loader::get_global_ptr() ),
	_event_mgr( EventHandler::get_global_event_handler() ),
	_vfs( VirtualFileSystem::get_global_ptr() ),
	_quit( false )
{
	_global_clock = ClockObject::get_global_clock();
	_global_clock->set_mode( ClockObject::M_slave );

	_tickcount = 0;
	_frameticks = 0;
	_currentframetick = 0;
	_simticksthisframe = 0;
	_curtime = 0.0;
	_oldcurtime = 0.0;
	_realframetime = 0.0;
	_frametime = 0.0;
	_framecount = 0;
	_remainder = 0.0;
	_tickrate = 66;
	_intervalpertick = 1.0 / _tickrate;
	_interpolationamount = 0.0;
	_paused = false;
}

/**
 * Adds a game system that will update each tick or each frame.
 * If per_frame is true, the system is updated at the end of each frame,
 * after all the ticks have run. If per_frame is false (default), the
 * system is updated during each tick of a frame.
 */
void HostBase::add_game_system( IGameSystem *system, int sort, bool per_frame )
{
	GameSystemEntry entry;
	entry.system = system;
	entry.sort = sort;
	entry.per_frame = per_frame;
	_game_systems_sorted.insert_nonunique( entry );
	_game_systems[system->get_type()] = entry;
}

void HostBase::mount_multifile( const Filename &mfpath )
{
	_vfs->mount( mfpath, Filename( "." ), VirtualFileSystem::MF_read_only );
}

void HostBase::mount_multifiles()
{
	//mount_multifile( "engine.mf" );
}

void HostBase::load_cfg_file( const Filename &cfgpath )
{
	load_prc_file( cfgpath );
}

void HostBase::load_cfg_data( const std::string &data )
{
	load_prc_file_data( "", data );
}

void HostBase::load_cfg_files()
{
	load_cfg_file( "cfg/engine.cfg" );
}

bool HostBase::startup()
{
	mount_multifiles();
	load_cfg_files();

	if ( ConfigVariableBool( "want-pstats", false ) )
		PStatClient::connect();

	add_game_systems();

	// Now initialize all the game systems
	size_t count = _game_systems_sorted.size();
	for ( size_t i = 0; i < count; i++ )
	{
		IGameSystem *sys = _game_systems_sorted[i].system;
		if ( !sys->initialize() )
		{
			hostbase_cat.error()
				<< "Unable to initialize game system " << std::string( sys->get_name() ) << "\n";
		}
	}

	_quit = false;

	return true;
}

void HostBase::shutdown()
{
	int num_systems = (int)_game_systems_sorted.size();
	for ( int i = num_systems - 1; i >= 0; i++ )
	{
		_game_systems_sorted[i].system->shutdown();
	}
	_game_systems_sorted.clear();
	_game_systems.clear();

	_quit = true;
}

void HostBase::do_frame()
{
	double curtime = _global_clock->get_real_time();
	_frametime = curtime - _curtime;
	_realframetime = _frametime;
	_curtime = curtime;
	_framecount++;

	double prevremainder = _remainder;
	if ( prevremainder < 0.0 )
		prevremainder = 0.0;

	_remainder += _frametime;

	int numticks = 0;
	if ( _remainder >= _intervalpertick )
	{
		numticks = (int)( _remainder / _intervalpertick );
		// Round to the nearest ending tick in alternate ticks so the last
		// tick is always simulated prior to updating network data.
		if ( sv_alternateticks )
		{
			int starttick = _tickcount;
			int endtick = starttick + numticks;
			endtick = align_value( endtick, 2 );
			numticks = endtick - starttick;
		}
		_remainder -= numticks * _intervalpertick;
	}

	_interpolationamount = 0.0;
	_frameticks = numticks;
	_currentframetick = 0;
	_simticksthisframe = 1;

	for ( int i = 0; i < numticks; i++ )
	{
		_curtime = _intervalpertick * _tickcount;
		_frametime = _intervalpertick;
		_global_clock->set_frame_time( _curtime );
		_global_clock->set_dt( _intervalpertick );
		_global_clock->set_frame_count( _tickcount );

		PandaNode::reset_all_prev_transform();

		// Run all of our per-tick game systems
		size_t num_systems = _game_systems_sorted.size();
		for ( size_t isys = 0; isys < num_systems; isys++ )
		{
			GameSystemEntry &entry = _game_systems_sorted[isys];
			if ( !entry.per_frame )
			{
				entry.system->update( _frametime );
			}
		}

		_tickcount++;
		_currentframetick++;
		_simticksthisframe++;
	}

	_oldcurtime = _curtime;
	_curtime = _tickcount * _intervalpertick * _remainder;
	_frametime = _realframetime;
	_global_clock->set_frame_time( _curtime );
	_global_clock->set_dt( _frametime );

	// Now run all of our per-frame game systems
	size_t num_systems = _game_systems_sorted.size();
	for ( size_t isys = 0; isys < num_systems; isys++ )
	{

		GameSystemEntry &entry = _game_systems_sorted[isys];
		if ( entry.per_frame )
		{
			entry.system->update( _frametime );
		}
	}

	TransformState::garbage_collect();
	RenderState::garbage_collect();
}

void HostBase::run()
{
	while ( !_quit )
	{
		do_frame();
	}
}

DEFINE_TASK_FUNC( HostBase::physics_task )
{
	HostBase *self = (HostBase *)data;
	double dt = ClockObject::get_global_clock()->get_dt();
	
	return AsyncTask::DS_cont;
}


DEFINE_TASK_FUNC( HostBase::ival_task )
{
	InputDeviceManager::get_global_ptr()->update();
	return AsyncTask::DS_cont;
}