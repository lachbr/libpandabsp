#include "cl_input.h"
#include "usercmd.h"
#include "in_buttons.h"
#include <mouseWatcher.h>
#include "clientbase.h"
#include "cl_rendersystem.h"
#include "inputsystem.h"
#include <keyboardButton.h>
#include <gamepadButton.h>

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
	if ( !win->has_pointer( 0 ) )
	{
		return;
	}
	LVector2i center( win->get_x_size() / 2, win->get_y_size() / 2 );
	MouseData md = win->get_pointer( 0 );
	delta.set( md.get_x() - center.get_x(), md.get_y() - center.get_y() );
	win->move_pointer( 0, center.get_x(), center.get_y() );
}

static void init_keyboard_mappings( InputSystem::InputDeviceContext *ctx )
{
	InputSystem::CInputMapping forward;
	forward.mapping_type = InputSystem::INPUTMAPPING_BUTTON;
	forward.button = KeyboardButton::ascii_key( 'w' );
	ctx->mappings[IN_FORWARD] = forward;

	InputSystem::CInputMapping back;
	back.mapping_type = InputSystem::INPUTMAPPING_BUTTON;
	back.button = KeyboardButton::ascii_key( 's' );
	ctx->mappings[IN_BACK] = back;

	InputSystem::CInputMapping moveleft;
	moveleft.mapping_type = InputSystem::INPUTMAPPING_BUTTON;
	moveleft.button = KeyboardButton::ascii_key( 'a' );
	ctx->mappings[IN_MOVELEFT] = moveleft;

	InputSystem::CInputMapping moveright;
	moveright.mapping_type = InputSystem::INPUTMAPPING_BUTTON;
	moveright.button = KeyboardButton::ascii_key( 'd' );
	ctx->mappings[IN_MOVERIGHT] = moveright;

	InputSystem::CInputMapping left; // look left
	left.mapping_type = InputSystem::INPUTMAPPING_BUTTON;
	left.button = KeyboardButton::left();
	ctx->mappings[IN_LEFT] = left;

	InputSystem::CInputMapping right; // look right
	right.mapping_type = InputSystem::INPUTMAPPING_BUTTON;
	right.button = KeyboardButton::right();
	ctx->mappings[IN_RIGHT] = right;

	InputSystem::CInputMapping jump;
	jump.mapping_type = InputSystem::INPUTMAPPING_BUTTON;
	jump.button = KeyboardButton::space();
	ctx->mappings[IN_JUMP] = jump;

	InputSystem::CInputMapping duck;
	duck.mapping_type = InputSystem::INPUTMAPPING_BUTTON;
	duck.button = KeyboardButton::control();
	ctx->mappings[IN_DUCK] = duck;

	InputSystem::CInputMapping speed;
	speed.mapping_type = InputSystem::INPUTMAPPING_BUTTON;
	speed.button = KeyboardButton::shift();
	ctx->mappings[IN_SPEED] = speed;

	InputSystem::CInputMapping walk;
	walk.mapping_type = InputSystem::INPUTMAPPING_BUTTON;
	walk.button = KeyboardButton::alt();
	ctx->mappings[IN_WALK] = walk;
}

void CInput::init_device_mappings( InputSystem::InputDeviceContext *ctx )
{
	switch ( ctx->device->get_device_class() )
	{
	case InputDevice::DeviceClass::virtual_device:
		{
			if ( ctx->device->has_feature( InputDevice::Feature::keyboard ) )
			{
				init_keyboard_mappings( ctx );
			}
			break;
		}
	case InputDevice::DeviceClass::keyboard:
		{
			init_keyboard_mappings( ctx );
			break;
		}
	case InputDevice::DeviceClass::gamepad:
		{
			CInputMapping movex;
			movex.mapping_type = INPUTMAPPING_AXIS;
			movex.axis = InputDevice::Axis::right_x;
			ctx->mappings[AXIS_X] = movex;

			CInputMapping movey;
			movey.mapping_type = INPUTMAPPING_AXIS;
			movey.axis = InputDevice::Axis::right_y;
			ctx->mappings[AXIS_Y] = movey;

			CInputMapping lookx;
			lookx.mapping_type = INPUTMAPPING_AXIS;
			lookx.axis = InputDevice::Axis::left_x;
			ctx->mappings[AXIS_LOOK_X] = lookx;

			CInputMapping looky;
			looky.mapping_type = INPUTMAPPING_AXIS;
			looky.axis = InputDevice::Axis::left_y;
			ctx->mappings[AXIS_LOOK_Y] = looky;

			CInputMapping jump;
			jump.mapping_type = INPUTMAPPING_BUTTON;
			jump.button = GamepadButton::ltrigger();
			ctx->mappings[IN_JUMP] = jump;

			CInputMapping duck;
			duck.mapping_type = INPUTMAPPING_BUTTON;
			duck.button = GamepadButton::rstick();
			ctx->mappings[IN_DUCK] = duck;

			CInputMapping speed;
			speed.mapping_type = INPUTMAPPING_BUTTON;
			speed.button = GamepadButton::rshoulder();
			ctx->mappings[IN_SPEED] = speed;

			CInputMapping walk;
			walk.mapping_type = INPUTMAPPING_BUTTON;
			walk.button = GamepadButton::rtrigger();
			ctx->mappings[IN_WALK] = walk;
			
			CInputMapping forward;
			forward.mapping_type = INPUTMAPPING_BUTTON;
			forward.button = GamepadButton::dpad_up();
			ctx->mappings[IN_FORWARD] = forward;

			CInputMapping back;
			back.mapping_type = INPUTMAPPING_BUTTON;
			back.button = GamepadButton::dpad_down();
			ctx->mappings[IN_BACK] = back;

			CInputMapping moveleft;
			moveleft.mapping_type = INPUTMAPPING_BUTTON;
			moveleft.button = GamepadButton::dpad_left();
			ctx->mappings[IN_MOVELEFT] = moveleft;

			CInputMapping moveright;
			moveright.mapping_type = INPUTMAPPING_BUTTON;
			moveright.button = GamepadButton::dpad_right();
			ctx->mappings[IN_MOVERIGHT] = moveright;

			//CInputMapping attack1;
			//attack1.m

			break;
		}
	}
}
