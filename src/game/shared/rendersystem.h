#pragma once

#include "config_game_shared.h"
#include "igamesystem.h"
#include "shader_generator.h"

#include <pgTop.h>
#include <graphicsEngine.h>
#include <graphicsStateGuardian.h>
#include <graphicsPipe.h>
#include <graphicsPipeSelection.h>
#include <graphicsWindow.h>
#include <nodePath.h>
#include <windowProperties.h>
#include <lens.h>
#include <luse.h>

class MouseWatcher;

/**
 * Creates and manages all things related to rendering.
 */
class EXPORT_GAME_SHARED RenderSystem : public IGameSystem
{
	DECLARE_CLASS( RenderSystem, IGameSystem )
public:
	RenderSystem();

        virtual const char *get_name() const;

        virtual bool initialize();
	virtual void init_render();
	bool init_rendering();
	bool init_scene();
	bool init_camera();
	bool init_shaders();

	void set_render( const NodePath &render );
	void set_gui_mouse_watcher( MouseWatcher *watcher );

        virtual void shutdown();

        virtual void update( double frametime );

	INLINE LVector2i get_size() const
	{
		if ( !_graphics_window )
			return LVector2i::zero();

		return LVector2i( _graphics_window->get_x_size(),
				  _graphics_window->get_y_size() );
	}

	INLINE float get_aspect_ratio() const
	{
		if ( !_graphics_window )
			return 1.0f;

		return (float)_graphics_window->get_sbs_left_x_size() / (float)_graphics_window->get_sbs_left_y_size();
	}

	INLINE GraphicsEngine *get_graphics_engine() const
	{
		return _graphics_engine;
	}

	INLINE GraphicsStateGuardian *get_gsg() const
	{
		return _gsg;
	}

	INLINE GraphicsPipe *get_graphics_pipe() const
	{
		return _graphics_pipe;
	}

	INLINE GraphicsPipeSelection *get_graphics_pipe_selection() const
	{
		return _graphics_pipe_selection;
	}

	INLINE GraphicsWindow *get_graphics_window() const
	{
		return _graphics_window;
	}

	const NodePath &get_render() const;
	const NodePath &get_aspect2d() const;
	const NodePath &get_pixel2d() const;
	const NodePath &get_render2d() const;
	const NodePath &get_camera() const;
	const NodePath &get_cam() const;
	const NodePath &get_hidden() const;

	const NodePath &get_a2d_top_center() const;
	const NodePath &get_a2d_top_left() const;
	const NodePath &get_a2d_top_right() const;
	const NodePath &get_a2d_bottom_center() const;
	const NodePath &get_a2d_bottom_left() const;
	const NodePath &get_a2d_bottom_right() const;
	const NodePath &get_a2d_left_center() const;
	const NodePath &get_a2d_right_center() const;

	BSPShaderGenerator *get_shader_generator() const;

	void adjust_window_aspect_ratio( float ratio );

protected:
	virtual void window_event();
	static void handle_window_event( const Event *e, void *data );

protected:
        GraphicsEngine *_graphics_engine;
        PT( GraphicsStateGuardian ) _gsg;
        PT( GraphicsPipe ) _graphics_pipe;
        GraphicsPipeSelection *_graphics_pipe_selection;
        PT( GraphicsWindow ) _graphics_window;
        PT( BSPShaderGenerator ) _shader_generator;
	PT( Lens ) _cam_lens;

	WindowProperties _prev_window_props;

	bool _window_foreground;
	bool _window_minimized;
	float _old_aspect_ratio;

	NodePath _camera;
	NodePath _cam;
	NodePath _aspect2d;
	NodePath _render2d;
	NodePath _pixel2d;
	NodePath _render;
	NodePath _hidden;

	float _a2d_top;
	float _a2d_bottom;
	float _a2d_left;
	float _a2d_right;

	NodePath _a2d_background;
	NodePath _a2d_top_center;
	NodePath _a2d_top_center_ns;
	NodePath _a2d_bottom_center;
	NodePath _a2d_bottom_center_ns;
	NodePath _a2d_left_center;
	NodePath _a2d_left_center_ns;
	NodePath _a2d_right_center;
	NodePath _a2d_right_center_ns;

	NodePath _a2d_top_left;
	NodePath _a2d_top_left_ns;
	NodePath _a2d_top_right;
	NodePath _a2d_top_right_ns;
	NodePath _a2d_bottom_left;
	NodePath _a2d_bottom_left_ns;
	NodePath _a2d_bottom_right;
	NodePath _a2d_bottom_right_ns;
};

INLINE BSPShaderGenerator *RenderSystem::get_shader_generator() const
{
	return _shader_generator;
}

INLINE const NodePath &RenderSystem::get_render() const
{
	return _render;
}

INLINE const NodePath &RenderSystem::get_aspect2d() const
{
	return _aspect2d;
}

INLINE const NodePath &RenderSystem::get_render2d() const
{
	return _render2d;
}

INLINE const NodePath &RenderSystem::get_pixel2d() const
{
	return _pixel2d;
}

INLINE const NodePath &RenderSystem::get_camera() const
{
	return _camera;
}

INLINE const NodePath &RenderSystem::get_cam() const
{
	return _cam;
}

INLINE const NodePath &RenderSystem::get_hidden() const
{
	return _hidden;
}

INLINE const NodePath &RenderSystem::get_a2d_top_center() const
{
	return _a2d_top_center;
}

INLINE const NodePath &RenderSystem::get_a2d_top_left() const
{
	return _a2d_top_left;
}

INLINE const NodePath &RenderSystem::get_a2d_top_right() const
{
	return _a2d_top_right;
}

INLINE const NodePath &RenderSystem::get_a2d_bottom_center() const
{
	return _a2d_bottom_center;
}

INLINE const NodePath &RenderSystem::get_a2d_bottom_left() const
{
	return _a2d_bottom_left;
}

INLINE const NodePath &RenderSystem::get_a2d_bottom_right() const
{
	return _a2d_bottom_right;
}

INLINE const NodePath &RenderSystem::get_a2d_left_center() const
{
	return _a2d_left_center;
}

INLINE const NodePath &RenderSystem::get_a2d_right_center() const
{
	return _a2d_right_center;
}

/**
 * Sets the node that is used as the root of the visible 3D scene graph.
 */
INLINE void RenderSystem::set_render( const NodePath &render )
{
	_render = render;
}

INLINE void RenderSystem::set_gui_mouse_watcher( MouseWatcher *watcher )
{
	DCAST( PGTop, _aspect2d.node() )->set_mouse_watcher( watcher );
	DCAST( PGTop, _pixel2d.node() )->set_mouse_watcher( watcher );
}

INLINE const char *RenderSystem::get_name() const
{
        return "RenderSystem";
}