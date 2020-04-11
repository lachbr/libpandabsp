#include "brushentity.h"
#include "sv_bsploader.h"

#include "scenecomponent_shared.h"

IMPLEMENT_CLASS( CBrushEntity )

bool CBrushEntity::can_transition() const
{
	return false;
}

void CBrushEntity::init( entid_t entnum )
{
	BaseClass::init( entnum );
}

void CBrushEntity::spawn()
{
	BaseClass::spawn();

	int modelnum = svbsp->get_bsp_loader()->extract_modelnum( _bsp_entnum );
	if ( get_entnum() == 0 )
	{
		_scene->np = svbsp->get_bsp_loader()->get_model( modelnum );
	}
	else
	{
		svbsp->get_bsp_loader()->get_model( modelnum ).reparent_to( _scene->np );
	}
}
