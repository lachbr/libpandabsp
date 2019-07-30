#ifndef POSTPROCESS_PASS_H
#define POSTPROCESS_PASS_H

#include "config_bsp.h"
#include "postprocess/postprocess_defines.h"

#include <referenceCount.h>
#include <namable.h>
#include <frameBufferProperties.h>
#include <nodePath.h>
#include <aa_luse.h>
#include <graphicsBuffer.h>
#include <camera.h>

class PostProcess;

class PostProcessPass : public ReferenceCount, public Namable
{
	TypeDecl2( PostProcessPass, ReferenceCount, Namable );

PUBLISHED:
	PostProcessPass( PostProcess *pp, const std::string &name = "pass", int texture_output_bits = 0,
			 const FrameBufferProperties &fbprops = PostProcessPass::get_default_fbprops(),
			 bool force_size = false, const LVector2i &forced_size = LVector2i::zero(), bool div_size = false, int div = 1 );

	INLINE NodePath get_quad() const
	{
		return _quad_np;
	}

	LVector2i get_back_buffer_dimensions() const;

	INLINE NodePath get_camera() const
	{
		return _camera_np;
	}

	INLINE bool has_texture_bits( int bits ) const
	{
		return ( _output_texture_bits & bits ) != 0;
	}

	INLINE void set_div_size( bool div_size, int div )
	{
		_div_size = div_size;
		_div = div;
	}

	INLINE void set_forced_size( bool force_size, const LVector2i &forced_size )
	{
		_force_size = force_size;
		_forced_size = forced_size;
	}

	INLINE void set_framebuffer_properties( const FrameBufferProperties &fbprops )
	{
		_fbprops = fbprops;
	}

	INLINE void set_output_texture_bits( int bits )
	{
		_output_texture_bits = bits;
	}

	LVector2i get_corrected_size( const LVector2i &size );

	Texture *get_texture( int bit );

	INLINE Texture *get_color_texture()
	{
		return get_texture( bits_PASSTEXTURE_COLOR );
	}
	INLINE Texture *get_depth_texture()
	{
		return get_texture( bits_PASSTEXTURE_DEPTH );
	}

	virtual bool setup_buffer();
	virtual void setup_textures();
	virtual void setup_quad();
	virtual void setup_camera();
	virtual void setup_region();
	virtual void setup();

	virtual void update();
	virtual void window_event( GraphicsOutput *output );

	virtual void shutdown();

	static FrameBufferProperties get_default_fbprops();

protected:
	PT( Texture ) make_texture( const std::string &name );

protected:
	PostProcess *_pp;

	PT( GraphicsBuffer ) _buffer;
	PT( DisplayRegion ) _region;
	NodePath _camera_np;
	PT( Camera ) _camera;
	NodePath _quad_np;

	bool _force_size;
	LVector2i _forced_size;
	FrameBufferProperties _fbprops;
	bool _div_size;
	int _div;

	int _output_texture_bits;

	// Output textures from this pass
	PT( Texture ) _color_texture;
	PT( Texture ) _depth_texture;
	PT( Texture ) _aux0_texture;
	PT( Texture ) _aux1_texture;
	PT( Texture ) _aux2_texture;
	PT( Texture ) _aux3_texture;

	static FrameBufferProperties *_default_fbprops;
};

#endif // POSTPROCESS_PASS_H