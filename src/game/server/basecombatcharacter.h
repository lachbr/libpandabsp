#pragma once

#include "baseanimating.h"

class EXPORT_SERVER_DLL CBaseCombatCharacter : public CBaseAnimating
{
	DECLARE_CLASS( CBaseCombatCharacter, CBaseAnimating )
	DECLARE_SERVERCLASS()

public:
	CBaseCombatCharacter();

	virtual void init( entid_t entnum );

public:
	NetworkInt( _health );
	NetworkInt( _max_health );
};