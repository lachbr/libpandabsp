#include "intervalsystem.h"
#include <cIntervalManager.h>

bool IntervalSystem::initialize()
{
	_mgr = new CIntervalManager;

	return true;
}

void IntervalSystem::shutdown()
{
	if ( _mgr )
	{
		delete _mgr;
		_mgr = nullptr;
	}
}

void IntervalSystem::update( double frametime )
{
	if ( _mgr )
		_mgr->step();
}