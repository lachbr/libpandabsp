#pragma once

#include "config_game_shared.h"
#include "igamesystem.h"

#include <inputDeviceManager.h>
#include <nodePath.h>
#include <nodePathCollection.h>
#include <dataGraphTraverser.h>
#include <mouseWatcher.h>
#include <buttonThrower.h>

class HostBase;

class EXPORT_GAME_SHARED InputSystem : public IGameSystem
{
	DECLARE_CLASS( InputSystem, IGameSystem )
public:
	enum
	{
		// The different ways we can accept input
		INPUTMAPPING_UNKNOWN,
		INPUTMAPPING_BUTTON,
		INPUTMAPPING_AXIS,
	};

	class CInputMapping
	{
	public:
		CInputMapping() = default;

		int mapping_type;

		union
		{
			ButtonHandle button;
			InputDevice::Axis axis;
		};

		bool get_button_value() const;
		float get_axis_value() const;
	};

	// Represents the key mappings for a particular input device.
	class InputDeviceContext
	{
	public:
		InputDevice *device;
		SimpleHashMap<int, CInputMapping, int_hash> mappings;
	};

	InputSystem( HostBase *host );

	virtual const char *get_name() const;

	virtual bool initialize();
	virtual void shutdown();
	virtual void update( double frametime );

	// Override this to bind buttons on this device to your game-specific
	// buttons.
	virtual void init_device_mappings( InputDeviceContext *context )
	{
	}

	void attach_device( InputDevice *device, const char *prefix = nullptr, bool watch = false );
	void detach_device( InputDevice *device );

	// Returns the maximum button value for all devices
	// that have a mapping to the specified button_type.
	bool get_button_value( int button_type ) const;
	float get_axis_value( int axis_type ) const;

	InputDeviceManager *get_mgr() const;

protected:
	pvector<InputDeviceContext> _device_contexts;
	//pvector<CKeyMapping> _mappings;

	DataGraphTraverser _dgtrav;
	NodePath _dataroot;
	NodePathCollection _mouse_and_keyboard;
	NodePathCollection _mouse_watcher;
	NodePathCollection _kb_button_thrower;
	SimpleHashMap<InputDevice *, NodePath, pointer_hash> _input_devices;
	NodePathCollection _device_button_throwers;

	HostBase *_host;
	InputDeviceManager *_mgr;
};

INLINE const char *InputSystem::get_name() const
{
	return "InputSystem";
}

INLINE InputDeviceManager *InputSystem::get_mgr() const
{
	return _mgr;
}
