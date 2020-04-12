#pragma once

#include "usercmd.h"

#include <datagram.h>

static constexpr int MAX_CLIENT_CMDS = 90;

static constexpr int NUM_NEW_CMD_BITS = 4;
static constexpr int MAX_NEW_COMMANDS = ( ( 1 << NUM_NEW_CMD_BITS ) - 1 );

static constexpr int NUM_BACKUP_CMD_BITS = 3;
static constexpr int MAX_BACKUP_CMDS = ( ( 1 << NUM_BACKUP_CMD_BITS ) - 1 );

class CClientCMDManager
{
public:
	CClientCMDManager( );

	CUserCmd *get_next_command( int &command_number );
	void consider_send();

	CUserCmd *get_command( int n );

private:
	void send_cmd();
	bool should_send_command() const;
	bool write_usercmd_delta( Datagram &dg, int from, int to, bool isnewcommand );

public:
	int _commands_sent;
	int _lastoutgoingcommand;
	int _chokedcommands;
	float _next_cmd_time;

	CUserCmd *_commands;
};

INLINE CUserCmd *CClientCMDManager::get_command( int n )
{
	CUserCmd *cmd = _commands + ( n % MAX_CLIENT_CMDS );
	if ( cmd->commandnumber != n )
	{
		return nullptr;
	}
	return cmd;
}