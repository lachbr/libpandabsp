#ifndef WDXGRAPHICSWINDOW11_H
#define WDXGRAPHICSWINDOW11_H

#include "config_bsp.h"

#include <winGraphicsWindow.h>

class wdxGraphicsWindow11 : public WinGraphicsWindow
{
	TypeDecl( wdxGraphicsWindow11, WinGraphicsWindow );

public:
	wdxGraphicsWindow11( GraphicsEngine *engine, GraphicsPipe *pipe,
		const std::string &name,
		const FrameBufferProperties &fb_prop,
		const WindowProperties &win_prop,
		int flags,
		GraphicsStateGuardian *gsg,
		GraphicsOutput *host );
	~wdxGraphicsWindow11();
};

#endif // WDXGRAPHICSWINDOW11_H