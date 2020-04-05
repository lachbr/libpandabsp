#include "basecombatcharacter.h"

CBaseCombatCharacter::CBaseCombatCharacter() :
	CBaseAnimating()
{
}

void CBaseCombatCharacter::init( entid_t entnum )
{
	BaseClass::init( entnum );
	_max_health = 100;
	_health = 100;
}

IMPLEMENT_SERVERCLASS_ST( CBaseCombatCharacter )
	SendPropInt( PROPINFO( _max_health ) ),
	SendPropInt( PROPINFO( _health ) )
END_SEND_TABLE()