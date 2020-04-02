#pragma once

#include "config_game_shared.h"
#include <typedReferenceCount.h>

class IGameSystem : public TypedReferenceCount
{
public:
	virtual const char *get_name() const = 0;

	virtual bool initialize() = 0;
	virtual void shutdown() = 0;
	virtual void update( double frametime ) = 0;
};