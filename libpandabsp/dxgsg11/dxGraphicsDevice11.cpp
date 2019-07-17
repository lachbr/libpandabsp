#include "dxGraphicsDevice11.h"
#include "wdxGraphicsPipe11.h"

#include <d3d11.h>

TypeDef( DXGraphicsDevice11 );

DXGraphicsDevice11::DXGraphicsDevice11( wdxGraphicsPipe11 *pipe ) :
	GraphicsDevice( pipe ),
	_swap_chain( nullptr ),
	_device( nullptr ),
	_device_context( nullptr )
{
}