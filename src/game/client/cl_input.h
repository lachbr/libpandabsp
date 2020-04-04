#pragma once

#include "config_clientdll.h"
#include "inputsystem.h"
#include <referenceCount.h>

class CUserCmd;

class EXPORT_CLIENT_DLL CInput : public InputSystem
{
	DECLARE_CLASS( CInput, InputSystem );

public:
	CInput();

	virtual float get_walk_speed() const;
	virtual float get_norm_speed() const;
	virtual float get_run_speed() const;
	virtual float get_duck_speed() const;

	virtual void setup_mouse( CUserCmd *cmd );
	virtual void setup_move( CUserCmd *cmd );
	virtual void setup_view( CUserCmd *cmd );

	virtual void create_cmd( CUserCmd *cmd, int commandnumber,
				 float input_sample_frametime, bool active );

	virtual void set_enabled( bool enabled );

	bool is_enabled() const;

protected:
	bool _enabled;
};

inline bool CInput::is_enabled() const
{
	return _enabled;
}

extern EXPORT_CLIENT_DLL CInput *clinput;