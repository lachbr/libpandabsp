#pragma once

#include "config_clientdll.h"
#include "rendersystem.h"
#include "game_postprocess.h"

class EXPORT_CLIENT_DLL ClientRenderSystem : public RenderSystem
{
	DECLARE_CLASS( ClientRenderSystem, RenderSystem )
public:
	ClientRenderSystem();

	virtual const char *get_name() const;
	virtual bool initialize();
	virtual void shutdown();
	virtual void update( double frametime );
	virtual void init_render();

	virtual void window_event();

private:
	PT( GamePostProcess ) _post_process;
};

INLINE void ClientRenderSystem::window_event()
{
	RenderSystem::window_event();
	_post_process->window_event();
}

INLINE const char *ClientRenderSystem::get_name() const
{
	return "ClientRenderSystem";
}

extern EXPORT_CLIENT_DLL ClientRenderSystem *clrender;