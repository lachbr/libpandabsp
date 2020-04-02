#pragma once

#include "datagramIterator.h"

class IClientDatagramHandler
{
public:
	virtual bool handle_datagram( int msg_type, DatagramIterator &dgi ) = 0;
};