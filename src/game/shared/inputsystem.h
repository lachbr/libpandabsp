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
	class CKeyMapping
	{
	public:
		CKeyMapping();
		CKeyMapping( int button_type, const ButtonHandle &btn );

		int button_type;
		ButtonHandle button;
		InputSystem *sys;

		bool is_down() const;
	};

	InputSystem( HostBase *host );

	virtual const char *get_name() const;

	virtual bool initialize();
	virtual void shutdown();
	virtual void update( double frametime );

	void attach_device( InputDevice *device, const char *prefix = nullptr, bool watch = false );
	void detach_device( InputDevice *device );

	void add_mapping( int button_type, const ButtonHandle &btn );
	size_t get_num_mappings() const;
	CKeyMapping &get_mapping( int button_type );

	InputDeviceManager *get_mgr() const;

protected:
	pvector<CKeyMapping> _mappings;

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

INLINE size_t InputSystem::get_num_mappings() const
{
	return _mappings.size();
}

INLINE InputSystem::CKeyMapping &InputSystem::get_mapping( int button_type )
{
	return _mappings[button_type];
}

INLINE const char *InputSystem::get_name() const
{
	return "InputSystem";
}

INLINE InputDeviceManager *InputSystem::get_mgr() const
{
	return _mgr;
}

INLINE InputSystem::CKeyMapping::CKeyMapping() :
	button_type( 0 ),
	button( 0 )
{
}

INLINE InputSystem::CKeyMapping::CKeyMapping( int type, const ButtonHandle &btn ) :
	button_type( type ),
	button( btn )
{
}