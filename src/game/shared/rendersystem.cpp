#include "rendersystem.h"

// Panda
#include <notifyCategoryProxy.h>
#include <displayInformation.h>
#include <pgTop.h>
#include <modelRoot.h>
#include <orthographicLens.h>
#include <perspectiveLens.h>
#include <configVariableInt.h>
#include <eventHandler.h>

// Shaders
#include "shader_vertexlitgeneric.h"
#include "shader_lightmappedgeneric.h"
#include "shader_unlitgeneric.h"
#include "shader_unlitnomat.h"
#include "shader_csmrender.h"
#include "shader_skybox.h"
#include "shader_decalmodulate.h"

#include "bsp_render.h"

static ConfigVariableInt main_camera_fov( "fov", 70 );

NotifyCategoryDeclNoExport( rendersystem )
NotifyCategoryDef( rendersystem, "" )

IMPLEMENT_CLASS( RenderSystem )

RenderSystem::RenderSystem() :
	_graphics_engine( nullptr ),
	_graphics_pipe( nullptr ),
	_gsg( nullptr ),
	_graphics_window( nullptr ),
	_graphics_pipe_selection( nullptr ),
	_cam_lens( nullptr ),
	_shader_generator( nullptr )
{
}

void RenderSystem::update( double frametime )
{
	if ( _shader_generator )
	{
		_shader_generator->update();
	}

	if ( _graphics_engine )
	{
		_graphics_engine->render_frame();
	}
}

bool RenderSystem::initialize()
{
	if ( !init_rendering() )
	{
		return false;
	}

	if ( !init_scene() )
	{
		return false;
	}

	if ( !init_camera() )
	{
		return false;
	}

	if ( !init_shaders() )
	{
		return false;
	}

	return true;
}

bool RenderSystem::init_rendering()
{
	//
	// Rendering
	//

	_graphics_pipe_selection = GraphicsPipeSelection::get_global_ptr();
	_graphics_engine = GraphicsEngine::get_global_ptr();

	_graphics_pipe = _graphics_pipe_selection->make_default_pipe();
	if ( !_graphics_pipe )
	{
		nassert_raise( "Could not create a graphics pipe!" );
		return false;
	}

	int window_flags = GraphicsPipe::BF_fb_props_optional | GraphicsPipe::BF_require_window;
	_graphics_window = DCAST(
		GraphicsWindow,
		_graphics_engine->make_output(
			_graphics_pipe, "MainWindow", 0, FrameBufferProperties::get_default(),
			WindowProperties::get_default(), window_flags ) );
	if ( !_graphics_window )
	{
		nassert_raise( "Could not open main window!" );
		return false;
	}

	_graphics_engine->open_windows();

	// Leave clearing up to the individual display regions.
	_graphics_window->disable_clears();

	_gsg = _graphics_window->get_gsg();
	nassertr( _gsg != nullptr, false );

	DisplayInformation *dinfo = _graphics_pipe->get_display_information();
	rendersystem_cat.info()
		<< "Graphics Information:\n"
		<< "\tVendor: " << _gsg->get_driver_vendor() << "\n"
		<< "\tRenderer: " << _gsg->get_driver_renderer() << "\n"
		<< "\tVersion: " << _gsg->get_driver_version() << "\n"
		<< "\tDevice ID: " << dinfo->get_device_id() << "\n"
		<< "\tVideo memory: " << dinfo->get_video_memory() << "\n"
		<< "\tTexture memory: " << dinfo->get_texture_memory() << "\n";

	EventHandler::get_global_event_handler()
		->add_hook( "window-event", handle_window_event, this );

	return true;
}

void RenderSystem::init_render()
{
	_render = NodePath( "render" );
	_render.set_attrib( RescaleNormalAttrib::make_default() );
	_render.set_two_sided( false );
}

bool RenderSystem::init_scene()
{
	//
	// Scene roots
	//


	init_render();

	_hidden = NodePath( "hidden" );

	_render2d = NodePath( "render2d" );
	_render2d.set_depth_test( false );
	_render2d.set_depth_write( false );
	_render2d.set_material_off( true );
	_render2d.set_two_sided( true );

	_aspect2d = _render2d.attach_new_node( new PGTop( "aspect2d" ) );
	float aspect_ratio = get_aspect_ratio();
	_aspect2d.set_scale( 1.0f / aspect_ratio, 1.0f, 1.0f );

	_a2d_background = _aspect2d.attach_new_node( "a2dBackground" );

	_a2d_top = 1.0f;
	_a2d_bottom = -1.0f;
	_a2d_left = -aspect_ratio;
	_a2d_right = aspect_ratio;

	_a2d_top_center = _aspect2d.attach_new_node( "a2dTopCenter" );
	_a2d_top_center_ns = _aspect2d.attach_new_node( "a2dTopCenterNS" );
	_a2d_bottom_center = _aspect2d.attach_new_node( "a2dBottomCenter" );
	_a2d_bottom_center_ns = _aspect2d.attach_new_node( "a2dBottomCenterNS" );
	_a2d_left_center = _aspect2d.attach_new_node( "a2dLeftCenter" );
	_a2d_left_center_ns = _aspect2d.attach_new_node( "a2dLeftCenterNS" );
	_a2d_right_center = _aspect2d.attach_new_node( "a2dRightCenter" );
	_a2d_right_center_ns = _aspect2d.attach_new_node( "a2dRightCenterNS" );

	_a2d_top_left = _aspect2d.attach_new_node( "a2dTopLeft" );
	_a2d_top_left_ns = _aspect2d.attach_new_node( "a2dTopLeftNS" );
	_a2d_top_right = _aspect2d.attach_new_node( "a2dTopRight" );
	_a2d_top_right_ns = _aspect2d.attach_new_node( "a2dTopRightNS" );
	_a2d_bottom_left = _aspect2d.attach_new_node( "a2dBottomLeft" );
	_a2d_bottom_left_ns = _aspect2d.attach_new_node( "a2dBottomLeftNS" );
	_a2d_bottom_right = _aspect2d.attach_new_node( "a2dBottomRight" );
	_a2d_bottom_right_ns = _aspect2d.attach_new_node( "a2dBottomRightNS" );

	_a2d_top_center.set_pos( 0, 0, _a2d_top );
	_a2d_top_center_ns.set_pos( 0, 0, _a2d_top );
	_a2d_bottom_center.set_pos( 0, 0, _a2d_bottom );
	_a2d_bottom_center_ns.set_pos( 0, 0, _a2d_bottom );
	_a2d_left_center.set_pos( _a2d_left, 0, 0 );
	_a2d_left_center_ns.set_pos( _a2d_left, 0, 0 );
	_a2d_right_center.set_pos( _a2d_right, 0, 0 );
	_a2d_right_center_ns.set_pos( _a2d_right, 0, 0 );

	_a2d_top_left.set_pos( _a2d_left, 0, _a2d_top );
	_a2d_top_left_ns.set_pos( _a2d_left, 0, _a2d_top );
	_a2d_top_right.set_pos( _a2d_right, 0, _a2d_top );
	_a2d_top_right_ns.set_pos( _a2d_right, 0, _a2d_top );
	_a2d_bottom_left.set_pos( _a2d_left, 0, _a2d_bottom );
	_a2d_bottom_left_ns.set_pos( _a2d_left, 0, _a2d_bottom );
	_a2d_bottom_right.set_pos( _a2d_right, 0, _a2d_bottom );
	_a2d_bottom_right_ns.set_pos( _a2d_right, 0, _a2d_bottom );

	_pixel2d = _render2d.attach_new_node( new PGTop( "pixel2d" ) );
	_pixel2d.set_pos( -1, 0, 1 );
	LVector2i size = get_size();
	if ( size[0] > 0 && size[1] > 0 )
		_pixel2d.set_scale( 2.0f / size[0], 1.0f, 2.0f / size[1] );

	// Enable shader generation on all of the main scenes
	if ( _gsg->get_supports_basic_shaders() && _gsg->get_supports_glsl() )
	{
		_render.set_shader_auto();
		_render2d.set_shader_auto();
	}
	else
	{
		// I don't know how this could be possible.
		nassert_raise( "GLSL shaders unsupported by graphics driver" );
		return false;
	}

	return true;
}

bool RenderSystem::init_camera()
{
	//
	// Camera
	//

	_camera = NodePath( new ModelRoot( "camera" ) );
	DCAST( ModelRoot, _camera.node() )->set_preserve_transform( ModelRoot::PT_local );
	_camera.reparent_to( _render );
	PT( DisplayRegion ) region = _graphics_window->make_display_region();
	// Only clear depth in the 3D scene. We don't expect any voids in the 3D
	// scene so we can save some frame time by not clearing color.
	region->disable_clears();
	region->set_clear_depth_active( true );
	PT( PerspectiveLens ) lens = new PerspectiveLens;
	lens->set_aspect_ratio( get_aspect_ratio() );
	PT( Camera ) camera = new Camera( "MainCamera", lens );
	_cam = NodePath( camera );
	_cam.reparent_to( _camera );
	region->set_camera( _cam );
	_cam_lens = lens;

	camera->set_camera_mask( CAMERA_MAIN );

	lens->set_min_fov( main_camera_fov / ( 4. / 3. ) );

	//
	// 2D Camera
	//

	PT( DisplayRegion ) dr2d = _graphics_window->make_display_region();
	dr2d->set_sort( 10 );
	// We don't need to clear anything when rendering 2D elements.
	// Color doesn't have to be cleared because we expect to be drawing
	// 2D elements on top of the 3D scene. Depth doesn't have to be
	// cleared because 2D is all the same depth.
	dr2d->disable_clears();
	PT( Camera ) camera2d = new Camera( "camera2d" );
	NodePath camera2dnp = _render2d.attach_new_node( camera2d );
	PT( Lens ) lens2d = new OrthographicLens;
	static const PN_stdfloat left = -1.0f;
	static const PN_stdfloat right = 1.0f;
	static const PN_stdfloat bottom = -1.0f;
	static const PN_stdfloat top = 1.0f;
	lens2d->set_film_size( right - left, top - bottom );
	lens2d->set_film_offset( ( right + left ) * 0.5, ( top + bottom ) * 0.5 );
	lens2d->set_near_far( -1000, 1000 );
	camera2d->set_lens( lens2d );
	dr2d->set_camera( camera2dnp );

	return true;
}

bool RenderSystem::init_shaders()
{
	//
	// Shaders
	//

	_shader_generator = new BSPShaderGenerator( _graphics_window, _gsg, _camera, _render );
	_gsg->set_shader_generator( _shader_generator );
	//_shader_generator->start_update();
	_shader_generator->add_shader( new VertexLitGenericSpec );
	_shader_generator->add_shader( new UnlitGenericSpec );
	_shader_generator->add_shader( new UnlitNoMatSpec );
	_shader_generator->add_shader( new LightmappedGenericSpec );
	_shader_generator->add_shader( new CSMRenderSpec );
	_shader_generator->add_shader( new SkyBoxSpec );
	_shader_generator->add_shader( new DecalModulateSpec );

	return true;
}

void RenderSystem::shutdown()
{
	EventHandler::get_global_event_handler()
		->remove_hook( "window-event", handle_window_event, this );

	_graphics_window->remove_all_display_regions();
	_graphics_engine->remove_all_windows();
	_graphics_window = nullptr;
	_gsg = nullptr;
	_graphics_pipe = nullptr;
	_graphics_pipe_selection = nullptr;

	_shader_generator = nullptr;

	_a2d_top_left.remove_node();
	_a2d_top_left_ns.remove_node();
	_a2d_top_right.remove_node();
	_a2d_top_right_ns.remove_node();
	_a2d_top_center.remove_node();
	_a2d_top_center_ns.remove_node();
	_a2d_bottom_center.remove_node();
	_a2d_bottom_center_ns.remove_node();
	_a2d_bottom_left.remove_node();
	_a2d_bottom_left_ns.remove_node();
	_a2d_bottom_right.remove_node();
	_a2d_bottom_right_ns.remove_node();
	_a2d_left_center.remove_node();
	_a2d_left_center_ns.remove_node();
	_a2d_right_center.remove_node();
	_a2d_right_center_ns.remove_node();
	_a2d_background.remove_node();

	_pixel2d.remove_node();
	_aspect2d.remove_node();
	_render2d.remove_node();

	_cam.remove_node();
	_camera.remove_node();

	_cam_lens = nullptr;
}

void RenderSystem::window_event()
{
	WindowProperties props = _graphics_window->get_properties();
	if ( props != _prev_window_props )
	{
		_prev_window_props = props;

		if ( !props.get_open() )
		{
			std::cout << "User closed main window." << std::endl;
			shutdown();
			return;
		}

		if ( props.get_foreground() && !_window_foreground )
			_window_foreground = true;
		else if ( !props.get_foreground() && _window_foreground )
			_window_foreground = false;

		if ( props.get_minimized() && !_window_minimized )
			_window_minimized = true;
		else if ( !props.get_minimized() && _window_minimized )
			_window_minimized = false;

		adjust_window_aspect_ratio( get_aspect_ratio() );

		if ( _graphics_window->has_size() && _graphics_window->get_sbs_left_y_size() != 0 )
		{
			_pixel2d.set_scale( 2.0f / _graphics_window->get_sbs_left_x_size(),
					    1.0f,
					    2.0f / _graphics_window->get_sbs_left_y_size() );
		}
		else
		{
			LVector2i size = get_size();
			if ( size[0] > 0 && size[1] > 0 )
			{
				_pixel2d.set_scale( 2.0f / size[0], 1.0f, 2.0f / size[1] );
			}
		}
	}
}

void RenderSystem::handle_window_event( const Event *e, void *data )
{
	RenderSystem *self = (RenderSystem *)data;
	self->window_event();
}

void RenderSystem::adjust_window_aspect_ratio( float ratio )
{
	if ( ratio != _old_aspect_ratio )
	{
		_old_aspect_ratio = ratio;

		if ( _cam_lens )
		{
			_cam_lens->set_aspect_ratio( ratio );
		}

		if ( ratio < 1.0f )
		{
			// Window is tall
			_aspect2d.set_scale( 1.0f, ratio, ratio );
			_a2d_top = 1.0f / ratio;
			_a2d_bottom = -1.0f / ratio;
			_a2d_right = 1.0f;
			_a2d_left = -1.0f;
		}
		else
		{
			// Window is wide
			_aspect2d.set_scale( 1.0f / ratio, 1.0f, 1.0f );
			_a2d_top = 1.0f;
			_a2d_bottom = -1.0f;
			_a2d_left = -ratio;
			_a2d_right = ratio;
		}

		// Reposition the aspect2d marker nodes
		_a2d_top_center.set_pos( 0, 0, _a2d_top );
		_a2d_top_center_ns.set_pos( 0, 0, _a2d_top );
		_a2d_bottom_center.set_pos( 0, 0, _a2d_bottom );
		_a2d_bottom_center_ns.set_pos( 0, 0, _a2d_bottom );
		_a2d_left_center.set_pos( _a2d_left, 0, 0 );
		_a2d_left_center_ns.set_pos( _a2d_left, 0, 0 );
		_a2d_right_center.set_pos( _a2d_right, 0, 0 );
		_a2d_right_center_ns.set_pos( _a2d_right, 0, 0 );

		_a2d_top_left.set_pos( _a2d_left, 0, _a2d_top );
		_a2d_top_left_ns.set_pos( _a2d_left, 0, _a2d_top );
		_a2d_top_right.set_pos( _a2d_right, 0, _a2d_top );
		_a2d_top_right_ns.set_pos( _a2d_right, 0, _a2d_top );
		_a2d_bottom_left.set_pos( _a2d_left, 0, _a2d_bottom );
		_a2d_bottom_left_ns.set_pos( _a2d_left, 0, _a2d_bottom );
		_a2d_bottom_right.set_pos( _a2d_right, 0, _a2d_bottom );
		_a2d_bottom_right_ns.set_pos( _a2d_right, 0, _a2d_bottom );
	}
}