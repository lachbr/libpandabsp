#include "tasksystem.h"

IMPLEMENT_CLASS( TaskSystem )

bool TaskSystem::initialize()
{
	_taskmgr = new AsyncTaskManager( "tasksystem-manager" );
	return true;
}

void TaskSystem::update( double frametime )
{
	if ( _taskmgr )
		_taskmgr->poll();
}

void TaskSystem::shutdown()
{
	if ( _taskmgr )
	{
		_taskmgr->cleanup();
		_taskmgr = nullptr;
	}
}

IMPLEMENT_CLASS( TaskSystemPerFrame )