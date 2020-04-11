#include "c_playercomponent.h"
#include "cl_netinterface.h"
#include "cl_entitymanager.h"

C_PlayerComponent::C_PlayerComponent() :
	C_BaseComponent()
{
	tickbase = 0;
	simulation_tick = -1;
}

void C_PlayerComponent::update( double frametime )
{
	clnet->cmd_tick();
}

void C_PlayerComponent::spawn()
{
	if ( clents->get_local_player_id() == _entity->get_entnum() )
	{
		clents->set_local_player( (C_BaseEntity *)_entity );
	}
}

IMPLEMENT_CLIENTCLASS_RT_NOBASE(C_PlayerComponent, CPlayerComponent)
	RecvPropInt(PROPINFO(tickbase)),
	RecvPropInt(PROPINFO(simulation_tick)),
END_RECV_TABLE()
