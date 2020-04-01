#pragma once

#include "config_game_shared.h"
#include <referenceCount.h>

class CEntityThinkFunc;

typedef void( *entitythinkfunc_t )( CEntityThinkFunc *func, void *data );

static constexpr int ENTITY_THINK_NEVER = -3;
static constexpr int ENTITY_THINK_ALWAYS = -2;

class EXPORT_GAME_SHARED CEntityThinkFunc : public ReferenceCount
{
public:
	int sort;
	entitythinkfunc_t func;
	void *data;
	float last_execute_time;
	float next_execute_time;
};