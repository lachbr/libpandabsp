#include "baseentity_shared.h"
#include <clockObject.h>

#include <modelRoot.h>

CBaseEntityShared::CBaseEntityShared() :
	_spawned( false ),
	_np( new ModelRoot( "entity" ) ),
	_entnum( 0 ),
	_main_think( nullptr ),
	_think_sort( 0 )
{
}

void CBaseEntityShared::init( entid_t entnum )
{
	_entnum = entnum;
}

void CBaseEntityShared::precache()
{
}

void CBaseEntityShared::spawn()
{
	make_think_func( "simulate", simulate_func, this );
	make_think_func( "think", think_func, this );

	_spawned = true;
}

void CBaseEntityShared::despawn()
{
	_spawned = false;

	_thinks.clear();
}

void CBaseEntityShared::simulate()
{
}

void CBaseEntityShared::simulate_func( CEntityThinkFunc *func, void *data )
{
	CBaseEntityShared *self = (CBaseEntityShared *)data;
	self->simulate();
	
	func->next_execute_time = ENTITY_THINK_ALWAYS;
}

void CBaseEntityShared::think()
{
}

void CBaseEntityShared::think_func( CEntityThinkFunc *func, void *data )
{
	CBaseEntityShared *ent = (CBaseEntityShared *)data;
	// Set next execute time before calling think() to allow user
	// to change it in their think() function.
	func->next_execute_time = ENTITY_THINK_ALWAYS;
	ent->think();
}

PT( CEntityThinkFunc ) CBaseEntityShared::make_think_func( const std::string &name, entitythinkfunc_t func, void *data, int sort )
{
	std::ostringstream ss;
	ss << name << "-entity-" << get_entnum();
	PT( CEntityThinkFunc ) thinkfunc = new CEntityThinkFunc;
	thinkfunc->data = data;
	thinkfunc->func = func;
	thinkfunc->next_execute_time = ENTITY_THINK_ALWAYS;
	if ( sort >= 0 )
	{
		thinkfunc->sort = sort;
	}
	else
	{
		thinkfunc->sort = _think_sort++;
	}
	_thinks.push_back( thinkfunc );

	if ( _thinks.size() > 1 )
	{
		std::sort( _thinks.begin(), _thinks.end(), []( const CEntityThinkFunc *a, const CEntityThinkFunc *b )
		{
			return a->sort < b->sort;
		} );
	}
	

	return thinkfunc;
}

void CBaseEntityShared::run_thinks()
{
	ClockObject *clock = ClockObject::get_global_clock();
	size_t num_thinks = _thinks.size();
	for ( size_t i = 0; i < num_thinks; i++ )
	{
		CEntityThinkFunc *thinkfunc = _thinks[i];
		if ( thinkfunc->next_execute_time != ENTITY_THINK_NEVER )
		{
			if ( thinkfunc->next_execute_time == ENTITY_THINK_ALWAYS ||
				( thinkfunc->next_execute_time <= clock->get_frame_time() &&
				  thinkfunc->last_execute_time < thinkfunc->next_execute_time ) )
			{
				thinkfunc->func( thinkfunc, thinkfunc->data );
				thinkfunc->last_execute_time = clock->get_frame_time();
			}
		}
	}
}