#pragma once

#include "config_clientdll.h"
#include "inputsystem.h"
#include <referenceCount.h>
#include <mouseData.h>

class CUserCmd;

class EXPORT_CLIENT_DLL CInput : public InputSystem
{
	DECLARE_CLASS( CInput, InputSystem );

public:
	CInput();

	void get_mouse_delta_and_center( LVector2f &delta );
};

extern EXPORT_CLIENT_DLL CInput *clinput;