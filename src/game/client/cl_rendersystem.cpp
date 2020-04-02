#include "cl_rendersystem.h"

#include <lightRampAttrib.h>

static ConfigVariableBool mat_hdr( "mat_hdr", true );
static ConfigVariableBool mat_bloom( "mat_bloom", true );
static ConfigVariableBool mat_fxaa( "mat_fxaa", true );

IMPLEMENT_CLASS( ClientRenderSystem )

ClientRenderSystem::ClientRenderSystem() :
	RenderSystem()
{
	_post_process = nullptr;
}

bool ClientRenderSystem::initialize()
{
	if ( !RenderSystem::initialize() )
	{
		return false;
	}

	_post_process = new GamePostProcess;
	_post_process->startup( _graphics_window );
	_post_process->add_camera( _cam );

	_post_process->_hdr_enabled = mat_hdr;
	_post_process->_bloom_enabled = mat_bloom;
	_post_process->_fxaa_enabled = mat_fxaa;

	if ( _post_process->_hdr_enabled )
		_render.set_attrib( LightRampAttrib::make_identity() );
	else
		_render.set_attrib( LightRampAttrib::make_default() );

	_post_process->setup();
}

void ClientRenderSystem::update( double frametime )
{
	_post_process->update();
}