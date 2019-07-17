#include "wdxGraphicsBuffer11.h"

TypeDef( wdxGraphicsBuffer11 );

wdxGraphicsBuffer11::wdxGraphicsBuffer11( GraphicsEngine *engine, GraphicsPipe *pipe,
	const std::string &name,
	const FrameBufferProperties &fb_prop,
	const WindowProperties &win_prop,
	int flags,
	GraphicsStateGuardian *gsg,
	GraphicsOutput *host ) :

	GraphicsBuffer( engine, pipe, name, fb_prop, win_prop, flags, gsg, host )
{
}