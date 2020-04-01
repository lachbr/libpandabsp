#include "c_baseplayer.h"
#include "globalvars_client.h"
#include "c_basegame.h"

C_BasePlayer::C_BasePlayer() :
	C_BaseCombatCharacter(),
	CBasePlayerShared()
{
}

void C_BasePlayer::simulate()
{

}

void C_BasePlayer::setup_controller()
{
	CBasePlayerShared::setup_controller();
	_np.reparent_to( _controller->get_movement_parent() );
	_np = _controller->get_movement_parent();
	std::cout << "Client controller setup" << std::endl;
	g_game->_render.ls();
}

void C_BasePlayer::spawn()
{
	BaseClass::spawn();

	setup_controller();

	// Check if we're the local player
	if ( g_globals->game->get_local_player_entnum() == get_entnum() )
	{
		g_globals->game->set_local_player( this );
		std::cout << "Set local player!" << std::endl;
	}
}

bool C_BasePlayer::is_local_player() const
{
	return g_globals->game->get_local_player() == this;
}

IMPLEMENT_CLIENTCLASS_RT( C_BasePlayer, DT_BasePlayer, CBasePlayer )
	RecvPropInt( RECVINFO( _tickbase ) )
END_RECV_TABLE()