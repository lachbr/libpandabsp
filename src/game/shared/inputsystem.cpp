#include "inputsystem.h"
#include "hostbase.h"
#include "rendersystem.h"

#include <mouseAndKeyboard.h>
#include <mouseWatcher.h>
#include <buttonThrower.h>

#include <inputDevice.h>
#include <inputDeviceNode.h>

NotifyCategoryDeclNoExport( inputsystem )
NotifyCategoryDef( inputsystem, "" )

IMPLEMENT_CLASS( InputSystem )

InputSystem::InputSystem( HostBase *host )
{
	_host = host;
	_mgr = nullptr;
}

bool InputSystem::initialize()
{
	RenderSystem *rsys;
	if ( !_host->get_game_system_of_type( rsys ) )
	{
		inputsystem_cat.error()
			<< "The input system requires the render system!\n";
		return false;
	}

	_mgr = InputDeviceManager::get_global_ptr();

	//
	// Data graph
	//

	_dataroot = NodePath( "dataroot" );

	GraphicsWindow *wind = rsys->get_graphics_window();

	for ( int i = 0; i < wind->get_num_input_devices(); i++ )
	{
		NodePath mak = _dataroot.attach_new_node( new MouseAndKeyboard( wind, i, wind->get_input_device_name( i ) ) );
		_mouse_and_keyboard.add_path( mak );

		// Watch the mouse
		std::ostringstream mwss;
		mwss << "watcher " << i;
		PT( MouseWatcher ) mw = new MouseWatcher( mwss.str() );

		if ( wind->get_side_by_side_stereo() )
		{
			mw->set_display_region( wind->get_overlay_display_region() );
		}

		ModifierButtons mmods = mw->get_modifier_buttons();
		mmods.add_button( KeyboardButton::shift() );
		mmods.add_button( KeyboardButton::control() );
		mmods.add_button( KeyboardButton::alt() );
		mmods.add_button( KeyboardButton::meta() );
		mw->set_modifier_buttons( mmods );
		NodePath mwnp = mak.attach_new_node( mw );
		_mouse_watcher.add_path( mwnp );

		// Watch for keyboard buttons
		std::ostringstream btss;
		btss << "buttons" << i;
		PT( ButtonThrower ) bt = new ButtonThrower( btss.str() );
		if ( i != 0 )
		{
			std::ostringstream press;
			press << "mousedev" << i << "-";
			bt->set_prefix( press.str() );
		}
		ModifierButtons mods;
		mods.add_button( KeyboardButton::shift() );
		mods.add_button( KeyboardButton::control() );
		mods.add_button( KeyboardButton::alt() );
		mods.add_button( KeyboardButton::meta() );
		bt->set_modifier_buttons( mods );
		NodePath btnp = mak.attach_new_node( bt );
		_kb_button_thrower.add_path( btnp );
	}

	if ( _mouse_watcher.get_num_paths() > 0 )
	{
		rsys->set_gui_mouse_watcher( DCAST( MouseWatcher, _mouse_watcher[0].node() ) );
	}

	return true;
}

void InputSystem::shutdown()
{
	_mgr = nullptr;

	_mouse_and_keyboard.clear();
	_mouse_watcher.clear();
	_kb_button_thrower.clear();

	_dataroot.remove_node();

	_mappings.clear();
}

void InputSystem::update( double frametime )
{
	_mgr->update();
	_dgtrav.traverse( _dataroot.node() );
}

void InputSystem::attach_device( InputDevice *device, const char *prefix, bool watch )
{
	// Protect against the same device being attached multiple times.
	if ( _input_devices.find( device ) != -1 )
	{
		return;
	}

	//device->map

	PT( InputDeviceNode ) idn = new InputDeviceNode( device, device->get_name() );
	NodePath idnp = _dataroot.attach_new_node( idn );

	if ( prefix || !watch )
	{
		PT( ButtonThrower ) bt = new ButtonThrower( device->get_name() );
		if ( prefix )
		{
			std::ostringstream press;
			press << std::string( prefix ) << "-";
			bt->set_prefix( press.str() );
		}
		_device_button_throwers.add_path( NodePath( bt ) );
	}

	_input_devices[device] = idnp;

	if ( watch )
	{
		idn->add_child( _mouse_watcher[0].node() );
	}
}

void InputSystem::detach_device( InputDevice *device )
{
	if ( _input_devices.find( device ) == -1 )
	{
		return;
	}

	NodePath idn = _input_devices[device];
	int count = _device_button_throwers.get_num_paths();
	for ( int num = count - 1; num >= 0; num-- )
	{
		NodePath bt = _device_button_throwers[num];
		if ( idn.is_ancestor_of( bt ) )
		{
			_device_button_throwers.remove_path( bt );
			break;
		}
	}

	idn.remove_node();
	_input_devices.remove( device );
}

void InputSystem::add_mapping( int button_type, const ButtonHandle &btn )
{
	CKeyMapping mapping( button_type, btn );
	mapping.sys = this;
	_mappings.push_back( mapping );
}

bool InputSystem::CKeyMapping::is_down() const
{
	MouseWatcher *mw = DCAST( MouseWatcher, sys->_mouse_watcher[0].node() );
	return mw->is_button_down( button );
}