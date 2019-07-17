#include "dxGraphicsStateGuardian11.h"

TypeDef( DXGraphicsStateGuardian11 );

DXGraphicsStateGuardian11::DXGraphicsStateGuardian11(GraphicsEngine *engine, GraphicsPipe *pipe ) :
	GraphicsStateGuardian( CoordinateSystem::CS_default, engine, pipe )
{
}

DXGraphicsStateGuardian11::~DXGraphicsStateGuardian11()
{
}