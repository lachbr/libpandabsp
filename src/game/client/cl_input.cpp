#include "cl_input.h"
#include "usercmd.h"
#include "in_buttons.h"
#include <mouseWatcher.h>
#include "clientbase.h"
#include "cl_rendersystem.h"
#include "inputsystem.h"

CInput *clinput = nullptr;

IMPLEMENT_CLASS( CInput )

CInput::CInput() :
	InputSystem( cl ),
	_enabled( false )
{
	clinput = this;

	_controls = install_player_controls();
}

PT( PlayerControls ) CInput::install_player_controls()
{
	return new PlayerControls;
}

void CInput::create_cmd( CUserCmd *cmd, int commandnumber, float input_sample_frametime, bool active )
{
	cmd->clear();

	cmd->commandnumber = commandnumber;
	cmd->tickcount = cl->get_tickcount();

	size_t count = get_num_mappings();
	for ( size_t i = 0; i < count; i++ )
	{
		const InputSystem::CKeyMapping &km = get_mapping( i );
		if ( km.is_down() )
			cmd->buttons |= km.button_type;
	}

	_controls->do_controls( cmd );
}

void CInput::set_enabled( bool enable )
{
	_enabled = enable;
}