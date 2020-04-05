#pragma once

#include "baseanimating.h"

class EXPORT_SERVER_DLL CBaseCombatCharacter : public CBaseAnimating
{
	DECLARE_SERVERCLASS( CBaseCombatCharacter, CBaseAnimating )

public:
	CBaseCombatCharacter();

	virtual void init( entid_t entnum );

public:
	NetworkInt( _health );
	NetworkInt( _max_health );
};