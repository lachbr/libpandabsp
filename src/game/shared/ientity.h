#pragma once

#include "config_game_shared.h"
#include "entityshared.h"

class EXPORT_GAME_SHARED IEntity
{
public:
	virtual entid_t get_entnum() const = 0;
};