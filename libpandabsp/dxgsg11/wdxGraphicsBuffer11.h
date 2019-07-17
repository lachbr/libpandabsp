#ifndef WDXGRAPHICSBUFFER11_H
#define WDXGRAPHICSBUFFER11_H

#include "config_bsp.h"

#include <graphicsBuffer.h>

class wdxGraphicsBuffer11 : public GraphicsBuffer
{
	TypeDecl( wdxGraphicsBuffer11, GraphicsBuffer );

public:
	wdxGraphicsBuffer11( GraphicsEngine *engine, GraphicsPipe *pipe,
		const std::string &name,
		const FrameBufferProperties &fb_prop,
		const WindowProperties &win_prop,
		int flags,
		GraphicsStateGuardian *gsg,
		GraphicsOutput *host );
	~wdxGraphicsBuffer11();
};

#endif // WDXGRAPHICSBUFFER11_H