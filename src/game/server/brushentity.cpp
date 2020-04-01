#include "brushentity.h"
#include "basegame.h"

bool CBrushEntity::can_transition() const
{
	return false;
}

void CBrushEntity::spawn()
{
	BaseClass::spawn();

	int modelnum = g_game->_bsp_loader->extract_modelnum( _bsp_entnum );
	if ( get_entnum() == 0 )
	{
		_np = g_game->_bsp_loader->get_model( modelnum );
	}
	else
	{
		g_game->_bsp_loader->get_model( modelnum ).reparent_to( _np );
	}
}

IMPLEMENT_SERVERCLASS_ST( CBrushEntity, DT_BrushEntity, brushentity )
END_SEND_TABLE()