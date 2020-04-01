#include "c_brushentity.h"
#include "c_basegame.h"

void C_BrushEntity::spawn()
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

IMPLEMENT_CLIENTCLASS_RT(C_BrushEntity, DT_BrushEntity, CBrushEntity)
END_RECV_TABLE()