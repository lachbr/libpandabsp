#pragma once

#include "config_clientdll.h"
#include "inputsystem.h"
#include <referenceCount.h>
#include "player_controls.h"

class CUserCmd;

class EXPORT_CLIENT_DLL CInput : public InputSystem
{
	DECLARE_CLASS( CInput, InputSystem );

public:
	CInput();

	virtual PT( PlayerControls ) install_player_controls();

	virtual void create_cmd( CUserCmd *cmd, int commandnumber,
				 float input_sample_frametime, bool active );

	virtual void set_enabled( bool enabled );

	bool is_enabled() const;

protected:
	PT( PlayerControls ) _controls;
	bool _enabled;
};

inline bool CInput::is_enabled() const
{
	return _enabled;
}

extern EXPORT_CLIENT_DLL CInput *clinput;