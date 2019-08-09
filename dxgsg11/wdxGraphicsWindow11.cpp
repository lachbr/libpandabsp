#include "wdxGraphicsWindow11.h"

TypeDef( wdxGraphicsWindow11 );

wdxGraphicsWindow11::wdxGraphicsWindow11( GraphicsEngine *engine, GraphicsPipe *pipe,
	const std::string &name,
	const FrameBufferProperties &fb_prop,
	const WindowProperties &win_prop,
	int flags,
	GraphicsStateGuardian *gsg,
	GraphicsOutput *host ) :

	WinGraphicsWindow( engine, pipe, name, fb_prop, win_prop, flags, gsg, host )
{

}