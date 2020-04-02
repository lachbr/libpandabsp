#pragma once

#include "config_game_shared.h"
#include "igamesystem.h"

class CIntervalManager;

class EXPORT_GAME_SHARED IntervalSystem : public IGameSystem
{
public:
	IntervalSystem();

	virtual const char *get_name() const;

	virtual bool initialize();
	virtual void shutdown();

	virtual void update( double frametime );

	CIntervalManager *get_mgr() const;

private:
	CIntervalManager *_mgr;
};

INLINE IntervalSystem::IntervalSystem()
{
	_mgr = nullptr;
}

INLINE const char *IntervalSystem::get_name() const
{
	return "IntervalSystem";
}

INLINE CIntervalManager *IntervalSystem::get_mgr() const
{
	return _mgr;
}