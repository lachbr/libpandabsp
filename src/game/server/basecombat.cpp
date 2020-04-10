#include "basecombat.h"

CBaseCombat::CBaseCombat() :
	CBaseComponent()
{
}

bool CBaseCombat::initialize()
{
	if ( !BaseClass::initialize() )
	{
		return false;
	}

	max_health = 100;
	health = 100;

	return true;
}

IMPLEMENT_SERVERCLASS_ST_NOBASE( CBaseCombat )
	SendPropInt( PROPINFO( max_health ) ),
	SendPropInt( PROPINFO( health ) ),
END_SEND_TABLE()