#ifndef WDXGRAPHICSPIPE11_H
#define WDXGRAPHICSPIPE11_H

#include "config_bsp.h"

#include <winGraphicsPipe.h>

#include <d3d11.h>

class wdxGraphicsPipe11 : public WinGraphicsPipe
{
	TypeDecl( wdxGraphicsPipe11, WinGraphicsPipe );

PUBLISHED:
	virtual std::string get_interface_name() const;

	virtual PT( GraphicsOutput ) make_output( const std::string &name,
		const FrameBufferProperties &fb_prop,
		const WindowProperties &win_prop,
		int flags,
		GraphicsEngine *engine,
		GraphicsStateGuardian *gsg,
		GraphicsOutput *host,
		int retry,
		bool &precertify );

public:
	static PT( GraphicsPipe ) pipe_constructor();

	virtual PT( GraphicsDevice ) make_device( void *scrn = nullptr );
	virtual PT( GraphicsStateGuardian ) make_callback_gsg( GraphicsEngine *engine );
};

#endif // WDXGRAPHICSPIPE11_H