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

IMPLEMENT_SERVERCLASS_ST( CBaseCombatCharacter, DT_BaseCombatCharacter, basecombatcharacter )
	SendPropInt( SENDINFO( _max_health ) ),
	SendPropInt( SENDINFO( _health ) )
END_SEND_TABLE()