#include "wdxGraphicsPipe11.h"
#include "dxGraphicsDevice11.h"
#include "dxGraphicsStateGuardian11.h"
#include "wdxGraphicsWindow11.h"
#include "wdxGraphicsBuffer11.h"

TypeDef( wdxGraphicsPipe11 );

std::string wdxGraphicsPipe11::get_interface_name() const
{
	return "DirectX11";
}

PT( GraphicsDevice ) wdxGraphicsPipe11::make_device( void *scrn )
{
	PT( DXGraphicsDevice11 ) device = new DXGraphicsDevice11( this );
	_device = device;

	return device;
}

PT( GraphicsPipe ) wdxGraphicsPipe11::pipe_constructor()
{
	return new wdxGraphicsPipe11;
}

PT( GraphicsOutput ) wdxGraphicsPipe11::make_output( const std::string &name,
	const FrameBufferProperties &fb_prop,
	const WindowProperties &win_prop,
	int flags,
	GraphicsEngine *engine,
	GraphicsStateGuardian *gsg,
	GraphicsOutput *host,
	int retry,
	bool &precertify )
{
	DXGraphicsStateGuardian11 *dxgsg = nullptr;
	if ( gsg )
	{
		DCAST_INTO_R( dxgsg, gsg, nullptr );
	}

	// First thing to try: a wdxGraphicsWindow11

	if ( retry == 0 )
	{
		if ( ( ( flags&BF_require_parasite ) != 0 ) ||
			( ( flags&BF_refuse_window ) != 0 ) ||
			( ( flags&BF_resizeable ) != 0 ) ||
			( ( flags&BF_size_track_host ) != 0 ) ||
			( ( flags&BF_rtt_cumulative ) != 0 ) ||
			( ( flags&BF_can_bind_color ) != 0 ) ||
			( ( flags&BF_can_bind_every ) != 0 ) )
		{
			return nullptr;
		}
		if ( ( flags & BF_fb_props_optional ) == 0 )
		{
			if ( ( fb_prop.get_aux_rgba() > 0 ) ||
				( fb_prop.get_aux_hrgba() > 0 ) ||
				( fb_prop.get_aux_float() > 0 ) )
			{
				return nullptr;
			}
		}
		return new wdxGraphicsWindow11( engine, this, name, fb_prop, win_prop,
			flags, gsg, host );
	}

	// Second thing to try: a wdxGraphicsBuffer11

	if ( retry == 1 )
	{
		if ( ( !support_render_texture ) ||
			( ( flags&BF_require_parasite ) != 0 ) ||
			( ( flags&BF_require_window ) != 0 ) ||
			( ( flags&BF_rtt_cumulative ) != 0 ) ||
			( ( flags&BF_can_bind_every ) != 0 ) )
		{
			return nullptr;
		}
		// Early failure - if we are sure that this buffer WONT meet specs, we can
		// bail out early.
		if ( ( flags & BF_fb_props_optional ) == 0 )
		{
			if ( fb_prop.get_indexed_color() ||
				( fb_prop.get_back_buffers() > 0 ) ||
				( fb_prop.get_accum_bits() > 0 ) ||
				( fb_prop.get_multisamples() > 0 ) )
			{
				return nullptr;
			}
		}

		// Early success - if we are sure that this buffer WILL meet specs, we can
		// precertify it.  This looks rather overly optimistic -- ie, buggy.
		if ( ( dxgsg != nullptr ) && dxgsg->is_valid() && !dxgsg->needs_reset() )//&&
			//dxgsg->get_supports_() )
		{
			precertify = true;
		}
		return new wdxGraphicsBuffer11( engine, this, name, fb_prop, win_prop,
			flags, gsg, host );
	}

	// Nothing else left to try.
	return nullptr;
}

PT( GraphicsStateGuardian ) wdxGraphicsPipe11::make_callback_gsg( GraphicsEngine *engine )
{
	return new DXGraphicsStateGuardian11( engine, this );
}