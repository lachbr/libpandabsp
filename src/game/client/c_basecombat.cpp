#include "c_basecombat.h"

C_BaseCombat::C_BaseCombat() :
	health( 0 ),
	max_health( 0 )
{
}

IMPLEMENT_CLIENTCLASS_RT_NOBASE( C_BaseCombat, CBaseCombat )
	RecvPropInt( PROPINFO( health ) ),
	RecvPropInt( PROPINFO( max_health ) )
END_RECV_TABLE()