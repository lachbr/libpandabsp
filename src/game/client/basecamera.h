#pragma once

#include "config_clientdll.h"
#include <referenceCount.h>
#include <aa_luse.h>

class CUserCmd;

class EXPORT_CLIENT_DLL CBaseCamera : public ReferenceCount
{
public:
	CBaseCamera();

	virtual void setup_camera();

	virtual void move_camera( CUserCmd *cmd, LVector3 &view_angles );
};
