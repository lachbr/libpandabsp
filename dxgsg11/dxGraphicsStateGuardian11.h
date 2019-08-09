#ifndef DXGRAPHICSSTATEGUARDIAN11_H
#define DXGRAPHICSSTATEGUARDIAN11_H

#include "config_bsp.h"

#include <graphicsStateGuardian.h>

class DXGraphicsStateGuardian11 : public GraphicsStateGuardian
{
	TypeDecl( DXGraphicsStateGuardian11, GraphicsStateGuardian );

public:
	DXGraphicsStateGuardian11( GraphicsEngine *engine, GraphicsPipe *pipe );
	~DXGraphicsStateGuardian11();
};

#endif // DXGRAPHICSSTATEGUARDIAN11_H