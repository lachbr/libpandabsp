#pragma once

#include "config_clientdll.h"
#include "bspsystem.h"
#include "iclientdatagramhandler.h"

#include <bsploader.h>

class EXPORT_CLIENT_DLL CL_BSPLoader : public BSPLoader
{
public:
	CL_BSPLoader();

protected:
	virtual void load_geometry();
	virtual void load_entities();
};

class EXPORT_CLIENT_DLL ClientBSPSystem : public BaseBSPSystem, public IClientDatagramHandler
{
	DECLARE_CLASS( ClientBSPSystem, BaseBSPSystem )

public:
	virtual bool handle_datagram( int msgtype, DatagramIterator &dgi );

	virtual const char *get_name() const;
	virtual bool initialize();
	virtual void load_bsp_level( const Filename &path, bool is_transition = false );
	virtual void cleanup_bsp_level();
};

INLINE const char *ClientBSPSystem::get_name() const
{
	return "ClientBSPSystem";
}
