#include "baseplayer.h"
#include "serverbase.h"
#include "sv_netinterface.h"
#include "sv_bsploader.h"
#include "physicssystem.h"
#include "playercomponent.h"


NotifyCategoryDef( baseplayer, "" )

IMPLEMENT_CLASS( CBasePlayer )

CBasePlayer::CBasePlayer() :
	CBaseEntity()
{
}

void CBasePlayer::init( entid_t entnum )
{
	BaseClass::init( entnum );

	//_view_offset = LVector3f( 0, 0, 4 );
}

void CBasePlayer::add_components()
{
	BaseClass::add_components();

	PT( CPlayerComponent ) player = new CPlayerComponent;
	add_component( player );
	_player_component = player;
}