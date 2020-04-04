#pragma once

#include "config_game_shared.h"
#include "igamesystem.h"

#include "asyncTask.h"
#include "asyncTaskManager.h"

/**
 * Use this system for tasks that you want to update every tick.
 */
class EXPORT_GAME_SHARED TaskSystem : public IGameSystem
{
	DECLARE_CLASS( TaskSystem, IGameSystem )

public:
	TaskSystem();

	// Methods of IGameSystem
	virtual const char *get_name() const;
	virtual bool initialize();
	virtual void shutdown();
	virtual void update( double frametime );

	void add_task( AsyncTask *task );
	AsyncTaskManager *get_task_mgr() const;

private:
	PT( AsyncTaskManager ) _taskmgr;
};

INLINE TaskSystem::TaskSystem()
{
	_taskmgr = nullptr;
}

INLINE void TaskSystem::add_task( AsyncTask *task )
{
	_taskmgr->add( task );
}

INLINE AsyncTaskManager *TaskSystem::get_task_mgr() const
{
	return _taskmgr;
}

INLINE const char *TaskSystem::get_name() const
{
	return "TaskSystem";
}

/**
 * Use this system for tasks that you want to update once each frame
 * before we render (after all the ticks).
 */
class EXPORT_GAME_SHARED TaskSystemPerFrame : public TaskSystem
{
	DECLARE_CLASS( TaskSystemPerFrame, TaskSystem )
public:
	virtual const char *get_name() const;
};

INLINE const char *TaskSystemPerFrame::get_name() const
{
	return "TaskSystemPerFrame";
}