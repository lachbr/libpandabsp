#include "bspsystem.h"
#include "bsploader.h"

IMPLEMENT_CLASS( BaseBSPSystem )

BaseBSPSystem::BaseBSPSystem()
{

}

void BaseBSPSystem::update( double frametime )
{
}

void BaseBSPSystem::shutdown()
{
	cleanup_bsp_level();
	_bsp_loader = nullptr;
}

void BaseBSPSystem::cleanup_bsp_level()
{
	_bsp_loader->cleanup();
}

Filename BaseBSPSystem::get_map_filename( const std::string &mapname )
{
	return Filename( "maps/" + mapname + ".bsp" );
}

void BaseBSPSystem::load_bsp_level( const Filename &path, bool is_transition )
{
	_map = path.get_basename_wo_extension();
	_bsp_loader->read( path, is_transition );
	_bsp_level = _bsp_loader->get_result();
}
