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
	InputSystem( cl )
{
	clinput = this;
}

void CInput::get_mouse_delta_and_center( LVector2f &delta )
{
	GraphicsWindow *win = clrender->get_graphics_window();
	LVector2i center( win->get_x_size() / 2, win->get_y_size() / 2 );
	MouseData md = win->get_pointer( 0 );
	delta.set( md.get_x() - center.get_x(), md.get_y() - center.get_y() );
	win->move_pointer( 0, center.get_x(), center.get_y() );
}