#include "client_commandmgr.h"
#include "clientbase.h"
#include "cl_netinterface.h"
#include "netmessages.h"
#include "cl_input.h"

CClientCMDManager::CClientCMDManager( ClientNetInterface *cl ) :
	_cl( cl ),
	_lastoutgoingcommand( -1 ),
	_commands_sent( 0 ),
	_next_cmd_time( 0.0f ),
	_chokedcommands( 0 )
{
	_commands = new CUserCmd[MAX_CLIENT_CMDS];
}

bool CClientCMDManager::should_send_command() const
{
	return cl->get_curtime() >= _next_cmd_time && _cl->_connected;
}

void CClientCMDManager::send_cmd()
{
	int nextcommandnr = _lastoutgoingcommand + _chokedcommands + 1;

	// Send the client update message

	Datagram dg = BeginMessage( NETMSG_USERCMD );

	static constexpr int cl_cmdbackup = 2;
	int backup_commands = clamp( cl_cmdbackup, 0, MAX_BACKUP_CMDS );
	dg.add_uint8( backup_commands );

	int new_commands = 1 + _chokedcommands;
	new_commands = clamp( new_commands, 0, MAX_NEW_COMMANDS );
	dg.add_uint8( new_commands );

	int numcmds = new_commands + backup_commands;

	int from = -1; // first command is deltaed against zeros

	bool ok = true;

	for ( int to = nextcommandnr - numcmds + 1; to <= nextcommandnr; to++ )
	{
		bool isnewcmd = to >= ( nextcommandnr - new_commands + 1 );

		// first valid command number is 1
		ok = ok && write_usercmd_delta( dg, from, to, isnewcmd );

		from = to;
	}

	if ( ok )
	{
		_cl->send_datagram( dg );
		_commands_sent++;
	}
}

bool CClientCMDManager::write_usercmd_delta( Datagram &dg, int from, int to, bool isnewcommand )
{
	CUserCmd nullcmd;

	CUserCmd *f, *t;

	if ( from == -1 )
	{
		f = &nullcmd;
	}
	else
	{
		f = get_command( from );
		if ( !f )
		{
			f = &nullcmd;
		}
	}

	t = get_command( to );

	if ( !t )
	{
		t = &nullcmd;
	}

	t->write_datagram( dg, f );

	return true;
}

void CClientCMDManager::tick()
{
	int nextcommandnr = _lastoutgoingcommand + _chokedcommands + 1;
	CUserCmd *cmd = &_commands[nextcommandnr % MAX_CLIENT_CMDS];
	clinput->create_cmd( cmd, nextcommandnr, cl->get_frametime(), true );

	bool send = should_send_command();

	if ( send )
	{
		send_cmd();

		_cl->send_tick();

		_lastoutgoingcommand = _commands_sent - 1;
		_chokedcommands = 0;

		// Determine when to send next command
		float cmd_ival = 1.0f / cl_cmdrate;
		float max_delta = std::min( (float)cl->get_interval_per_tick(), cmd_ival );
		float delta = clamp( (float)( cl->get_curtime() - _next_cmd_time ), 0.0f, max_delta );
		_next_cmd_time = cl->get_curtime() + cmd_ival - delta;
	}
	else
	{
		// Not sending yet, but building a list of commands to send.
		_chokedcommands++;
	}	
}