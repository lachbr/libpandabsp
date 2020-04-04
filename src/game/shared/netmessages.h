#pragma once

#include <netDatagram.h>

enum
{
	CLIENTSTATE_NONE,
	CLIENTSTATE_LOADING,
	CLIENTSTATE_PLAYING,
};

INLINE NetDatagram BeginMessage( int msgtype )
{
	NetDatagram dg;
	dg.add_uint16( msgtype );
	return dg;
}

enum
{
	NETMSG_SNAPSHOT,

	NETMSG_CLIENT_HELLO,
	NETMSG_HELLO_RESP,

	NETMSG_CLIENT_HEARTBEAT,
	NETMSG_SERVER_HEARTBEAT,

	NETMSG_CMD,

	NETMSG_CLIENT_STATE,
	NETMSG_CHANGE_LEVEL,

	NETMSG_ENTITY_MSG,

	NETMSG_DELETE_ENTITY,

	NETMSG_SYNC,
	NETMSG_SYNC_RESP,

	// CUserCmd sent from client to server
	NETMSG_USERCMD,

	// Sent from the server to each client, each frame,
	// to tell them what tick the server is on.
	NETMSG_TICK,
};