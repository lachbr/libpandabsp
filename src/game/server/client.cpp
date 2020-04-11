#include "client.h"
#include "serverbase.h"
#include "usercmd.h"
#include "baseplayer.h"
#include "playercomponent.h"
#include <datagramIterator.h>

NotifyCategoryDef( sv_client, "" )

void Client::handle_cmd( DatagramIterator &dgi )
{
	sv_client_cat.debug()
		<< "Received cmd message from client " << get_client_id() << "\n";

	if ( _last_movement_tick == sv->get_tickcount() )
	{
		// Only one movement command per frame.
		sv_client_cat.debug()
			<< "Received more than one command this tick" << std::endl;
		return;
	}

	_last_movement_tick = sv->get_tickcount();

	if ( !_player )
		return;

	int backup_commands = dgi.get_uint8();
	int new_commands = dgi.get_uint8();
	int totalcmds = new_commands + backup_commands;

	int i;
	CUserCmd *from, *to;

	static constexpr int CMD_MAXBACKUP = 64;

	// We rack last three commands in case we drop
	//  packets but get them back.
	CUserCmd cmds[CMD_MAXBACKUP];

	CUserCmd nullcmd; // for delta compression

	assert( new_commands >= 0 );
	assert( ( totalcmds - new_commands ) >= 0 );

	// Too many commands?
	if ( totalcmds < 0 || totalcmds >= ( CMD_MAXBACKUP - 1 ) )
	{
		std::cout << "ERROR: too many cmds " << totalcmds
			<< " sent from client " << get_client_id() << std::endl;
		return;
	}

	from = &nullcmd;
	for ( i = totalcmds - 1; i >= 0; i-- )
	{
		sv_client_cat.debug()
			<< "Reading cmd " << i << "\n";
		to = &cmds[i];
		to->read_datagram( dgi, from );
		from = to;
	}

	// TODO: paused
	CPlayerComponent *player_component;
	if ( _player->get_component_of_type( player_component ) )
	{
		player_component->process_usercmds( cmds, new_commands, totalcmds, false );
	}
}