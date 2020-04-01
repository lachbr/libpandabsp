#include "c_basecombatcharacter.h"

C_BaseCombatCharacter::C_BaseCombatCharacter() :
	_health( 0 ),
	_max_health( 0 )
{
}

IMPLEMENT_CLIENTCLASS_RT( C_BaseCombatCharacter, DT_BaseCombatCharacter, CBaseCombatCharacter )
	RecvPropInt( RECVINFO( _health ) ),
	RecvPropInt( RECVINFO( _max_health ) )
END_RECV_TABLE()