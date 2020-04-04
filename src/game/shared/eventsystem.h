#pragma once

#include "config_game_shared.h"
#include "igamesystem.h"

class EXPORT_GAME_SHARED EventSystem : public IGameSystem
{
	DECLARE_CLASS( EventSystem, IGameSystem );
public:
	virtual const char *get_name() const;
	virtual bool initialize();
	virtual void shutdown();
	virtual void update( double frametime );
};