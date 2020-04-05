#include "brushentity.h"
#include "sv_bsploader.h"

bool CBrushEntity::can_transition() const
{
	return false;
}

void CBrushEntity::spawn()
{
	BaseClass::spawn();

	int modelnum = svbsp->get_bsp_loader()->extract_modelnum( _bsp_entnum );
	if ( get_entnum() == 0 )
	{
		_np = svbsp->get_bsp_loader()->get_model( modelnum );
	}
	else
	{
		svbsp->get_bsp_loader()->get_model( modelnum ).reparent_to( _np );
	}
}

IMPLEMENT_SERVERCLASS_ST( CBrushEntity )
END_SEND_TABLE()