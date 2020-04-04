#pragma once

#include "igamesystem.h"
#include "nodePath.h"
#include "filename.h"

class BSPLoader;

class BaseBSPSystem : public IGameSystem
{
	DECLARE_CLASS( BaseBSPSystem, IGameSystem )

public:
	BaseBSPSystem();

	virtual void shutdown();
	virtual void update( double frametime );

	INLINE BSPLoader *get_bsp_loader() const
	{
		return _bsp_loader;
	}

	virtual void cleanup_bsp_level();
	virtual void load_bsp_level( const Filename &path, bool is_transition = false );
	virtual Filename get_map_filename( const std::string &mapname );

	const NodePath &get_bsp_level() const;
	const std::string &get_map() const;

protected:
	BSPLoader *_bsp_loader;
	NodePath _bsp_level;
	std::string _map;
};

INLINE const NodePath &BaseBSPSystem::get_bsp_level() const
{
	return _bsp_level;
}

INLINE const std::string &BaseBSPSystem::get_map() const
{
	return _map;
}
