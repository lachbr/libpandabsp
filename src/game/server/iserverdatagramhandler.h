#pragma once

#include "datagramIterator.h"

class Client;

class IServerDatagramHandler
{
public:
	virtual bool handle_datagram( Client *cl, int msg_type, DatagramIterator &dgi ) = 0;
};