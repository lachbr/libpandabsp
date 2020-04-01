#include "basegame_shared.h"
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

static ConfigVariableInt phys_substeps( "phys_substeps", 1 );

HostBase::HostBase() :
	_task_manager( AsyncTaskManager::get_global_ptr() ),
	_loader( Loader::get_global_ptr() ),
	_bsp_loader( nullptr ),
	_ival_mgr( CIntervalManager::get_global_ptr() ),
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

void HostBase::cleanup_bsp_level()
{
	if ( !_bsp_level.is_empty() )
	{
		detach_and_remove_bullet_nodes( _bsp_level );
		_bsp_level.remove_node();
	}
	_bsp_loader->cleanup();
}

Filename HostBase::get_map_filename( const std::string &mapname )
{
	return Filename( "maps/" + mapname + ".bsp" );
}

void HostBase::load_bsp_level( const Filename &path, bool is_transition )
{
	_map = path.get_basename_wo_extension();
	nassertv( _bsp_loader->read( path, is_transition ) );
	_bsp_level = _bsp_loader->get_result();
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

	make_bsp();
	setup_rendering();
	setup_scene();
	setup_camera();
	setup_dgraph();
	setup_physics();
	setup_audio();
	setup_tasks();
	setup_shaders();
	setup_bsp();

	_quit = false;

	return true;
}

void HostBase::setup_rendering()
{
}

void HostBase::setup_camera()
{
}

void HostBase::setup_dgraph()
{
}

void HostBase::setup_audio()
{
}

void HostBase::setup_shaders()
{
}

void HostBase::setup_tasks()
{
	//
	// Tasks
	//

	_reset_prev_transform_task = new GenericAsyncTask( "resetPrevTransform", reset_prev_transform_task, this );
	_reset_prev_transform_task->set_sort( -51 );
	_task_manager->add( _reset_prev_transform_task );

	_ival_task = new GenericAsyncTask( "ivalLoop", ival_task, this );
	_ival_task->set_sort( 20 );
	_task_manager->add( _ival_task );

	_physics_task = new GenericAsyncTask( "physicsUpdate", physics_task, this );
	_physics_task->set_sort( 30 );
	_task_manager->add( _physics_task );

	_garbage_collect_task = new GenericAsyncTask( "garbageCollectStates", garbage_collect_task, this );
	_garbage_collect_task->set_sort( 46 );
	_task_manager->add( _garbage_collect_task );
}

void HostBase::setup_scene()
{
	//
	// Scene roots
	//

	_render = NodePath( new BSPRender( "render", _bsp_loader ) );
	_render.set_attrib( RescaleNormalAttrib::make_default() );
	_render.set_two_sided( false );

	_hidden = NodePath( "hidden" );
}

void HostBase::setup_physics()
{
	//
	// Physics
	//

	_physics_world = new BulletWorld;
	// Panda units are in feet, so the gravity is 32 feet per second,
	// not 9.8 meters per second.
	_physics_world->set_gravity( 0.0f, 0.0f, -32.174f );
}

void HostBase::setup_bsp()
{
	//
	// BSP
	//
}

void HostBase::shutdown()
{
	_physics_world = nullptr;
	_render.remove_node();
	_hidden.remove_node();
	_bsp_loader->cleanup();

	_physics_task->remove();
	_physics_task = nullptr;
	_garbage_collect_task->remove();
	_garbage_collect_task = nullptr;
	_ival_task->remove();
	_ival_task = nullptr;
	_reset_prev_transform_task->remove();
	_reset_prev_transform_task = nullptr;

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

		// Handle network events and receive messages.
		do_net_tick();

		// Let the specific host implementation do per-tick logic.
		do_game_tick();

		_tickcount++;
		_currentframetick++;
		_simticksthisframe++;
	}

	_oldcurtime = _curtime;
	_curtime = _tickcount * _intervalpertick * _remainder;
	_frametime = _realframetime;
	_global_clock->set_frame_time( _curtime );
	_global_clock->set_dt( _frametime );
}

void HostBase::run()
{
	while ( !_quit )
	{
		do_frame();
	}
}

DEFINE_TASK_FUNC( HostBase::reset_prev_transform_task )
{
	PandaNode::reset_all_prev_transform();
	return AsyncTask::DS_cont;
}

DEFINE_TASK_FUNC( HostBase::physics_task )
{
	HostBase *self = (HostBase *)data;
	double dt = ClockObject::get_global_clock()->get_dt();
	if ( dt <= 0.016f )
	{
		self->_physics_world->do_physics( dt, phys_substeps, 0.016f );
	}
	else if ( dt <= 0.033f )
	{
		self->_physics_world->do_physics( dt, phys_substeps, 0.033f );
	}
	else
	{
		self->_physics_world->do_physics( dt, 0 );
	}
	return AsyncTask::DS_cont;
}


DEFINE_TASK_FUNC( HostBase::ival_task )
{
	InputDeviceManager::get_global_ptr()->update();
	CIntervalManager::get_global_ptr()->step();
	return AsyncTask::DS_cont;
}

DEFINE_TASK_FUNC( HostBase::garbage_collect_task )
{
	TransformState::garbage_collect();
	RenderState::garbage_collect();
	return AsyncTask::DS_cont;
}