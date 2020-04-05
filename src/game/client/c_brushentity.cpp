#include "c_brushentity.h"
#include "cl_bsploader.h"

void C_BrushEntity::spawn()
{
	BaseClass::spawn();

	int modelnum = clbsp->get_bsp_loader()->extract_modelnum( _bsp_entnum );
	if ( get_entnum() == 0 )
	{
		_np = clbsp->get_bsp_loader()->get_model( modelnum );
	}
	else
	{
		clbsp->get_bsp_loader()->get_model( modelnum ).reparent_to( _np );
	}
	
}

IMPLEMENT_CLIENTCLASS_RT( C_BrushEntity, CBrushEntity )
END_RECV_TABLE()