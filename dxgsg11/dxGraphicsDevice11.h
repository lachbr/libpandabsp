#ifndef DXGRAPHICSDEVICE11_H
#define DXGRAPHICSDEVICE11_H

#include "config_bsp.h"

#include <graphicsDevice.h>

#include <d3d11.h>

class wdxGraphicsPipe11;

class DXGraphicsDevice11 : public GraphicsDevice
{
	TypeDecl( DXGraphicsDevice11, GraphicsDevice );

public:
	DXGraphicsDevice11( wdxGraphicsPipe11 *pipe );
	~DXGraphicsDevice11();

	INLINE ID3D11Device *get_device()
	{
		return _device;
	}
	INLINE ID3D11DeviceContext *get_device_context()
	{
		return _device_context;
	}
	INLINE IDXGISwapChain *get_swap_chain()
	{
		return _swap_chain;
	}

private:
	ID3D11Device *_device;
	ID3D11DeviceContext *_device_context;
	IDXGISwapChain *_swap_chain;
};

#endif // DXGRAPHICSDEVICE11_H