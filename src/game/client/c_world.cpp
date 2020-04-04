#include "c_world.h"
#include "cl_bsploader.h"

void C_World::update_parent_entity( int entnum )
{
	if ( entnum == 0 )
	{
		_np.reparent_to( clbsp->get_bsp_level() );
	}
	else
	{
		BaseClass::update_parent_entity( entnum );
	}
}

IMPLEMENT_CLIENTCLASS_RT(C_World, DT_World, CWorld)
END_RECV_TABLE()