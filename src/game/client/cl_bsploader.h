#pragma once

#include "config_clientdll.h"

#include <bsploader.h>

class EXPORT_CLIENT_DLL CL_BSPLoader : public BSPLoader
{
public:
	CL_BSPLoader();

protected:
	virtual void load_geometry();
	virtual void load_entities();
};