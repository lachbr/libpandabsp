#include "keymappings.h"
#include "c_basegame.h"
#include <mouseWatcher.h>

CKeyMapping::CKeyMapping() :
	button_type( 0 )
{
}

CKeyMapping::CKeyMapping( int btype, const ButtonHandle &bhandle ) :
	button_type( btype ),
	button( bhandle )
{
}

bool CKeyMapping::is_down() const
{
	MouseWatcher *mw = DCAST( MouseWatcher, ClientBase::ptr()->_mouse_watcher[0].node() );
	return mw->is_button_down( button );
}

void CKeyMappings::add_mapping( int button_type, const ButtonHandle &btn )
{
	mappings[button_type] = CKeyMapping( button_type, btn );
}