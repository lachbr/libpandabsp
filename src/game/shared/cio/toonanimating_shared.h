#if 0
#pragma once

#include "config_game_shared.h"
#include "baseanimating_shared.h"

#ifdef CLIENT_DLL
#define ToonAnimating C_ToonAnimating
#else
#define ToonAnimating CToonAnimating
#endif

class EXPORT_GAME_SHARED ToonAnimating : public CBaseAnimating
{
	DECLARE_COMPONENT( ToonAnimating, CBaseAnimating )
};

#endif