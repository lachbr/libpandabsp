#include "c_playercomponent.h"
#include "cl_netinterface.h"
#include "cl_entitymanager.h"
#include "cl_input.h"

C_PlayerComponent::C_PlayerComponent() :
	C_BaseComponent(),
	iv_view_offset( "C_PlayerComponent_iv_view_offset" )
{
	tickbase = 0;
	simulation_tick = -1;

	add_var( &view_offset, &iv_view_offset, LATCH_SIMULATION_VAR );
}

void C_PlayerComponent::update( double frametime )
{
	// Build a user command.
	int command_number;
	CUserCmd *cmd = _cmd_mgr.get_next_command( command_number );

	cmd->clear();

	cmd->commandnumber = command_number;
	cmd->tickcount = cl->get_tickcount();

	//size_t count = clinput->get_num_mappings();
	//for ( size_t i = 0; i < count; i++ )
	//{
	//	const InputSystem::CKeyMapping &km = clinput->get_mapping( i );
	//	if ( km.is_down() )
	//		cmd->buttons |= km.button_type;
	//}

	create_cmd( cmd );
	setup_view( cmd );
	player_run_command( cmd, frametime );

	// Send the command if it's time to.
	_cmd_mgr.consider_send();
}

void C_PlayerComponent::spawn()
{
	if ( clents->get_local_player_id() == _entity->get_entnum() )
	{
		clents->set_local_player( (C_BaseEntity *)_entity );
	}
}

IMPLEMENT_CLIENTCLASS_RT_NOBASE( C_PlayerComponent, CPlayerComponent )
	RecvPropInt( PROPINFO( tickbase ) ),
	RecvPropInt( PROPINFO( simulation_tick ) ),
	RecvPropVec3( PROPINFO( view_offset ) ),
END_RECV_TABLE()
