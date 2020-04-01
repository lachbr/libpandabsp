#include "clientbase.h"
#include "physics_utils.h"
#include "netmessages.h"
#include "c_baseplayer.h"
#include "cl_bsploader.h"
#include "in_buttons.h"

#include "shader_generator.h"
#include "shader_vertexlitgeneric.h"
#include "shader_lightmappedgeneric.h"
#include "shader_unlitgeneric.h"
#include "shader_unlitnomat.h"
#include "shader_csmrender.h"
#include "shader_skybox.h"
#include "shader_decalmodulate.h"

#include <mouseAndKeyboard.h>
#include <keyboardButton.h>
#include <mouseButton.h>
#include <buttonThrower.h>
#include <camera.h>
#include <modelRoot.h>
#include <orthographicLens.h>
#include <textNode.h>
#include <dynamicTextFont.h>

#include <pgTop.h>

ClientBase *cl = nullptr;

static ConfigVariableInt main_camera_fov( "fov", 70 );
static ConfigVariableBool cl_showfps( "cl_showfps", false );
static ConfigVariableInt fps_max( "fps_max", 300 );

ClientBase::ClientBase() :
	HostBase(),
	_graphics_engine( GraphicsEngine::get_global_ptr() ),
	_input_mgr( InputDeviceManager::get_global_ptr() ),
	_graphics_pipe( nullptr ),
	_gsg( nullptr ),
	_graphics_window( nullptr ),
	_graphics_pipe_selection( GraphicsPipeSelection::get_global_ptr() ),
	_client( new ClientNetInterface ),
	_cam_lens( nullptr ),
	_post_process( nullptr ),
	_fps_meter( new CFrameRateMeter ),
	_local_player( nullptr ),
	_local_player_entnum( 0 ),
	_key_mappings( new CKeyMappings )
{
	cl = this;
}

void ClientBase::run_cmd( const std::string &full_cmd )
{
	vector_string args = parse_cmd( full_cmd );
	std::string cmd = args[0];
	args.erase( args.begin() );

	if ( cmd == "map" )
	{
		if ( args.size() > 0 )
		{
			std::string mapname = args[0];
			start_game( mapname );
		}
	}
	else
	{
		// Don't know this command from client,
		// send it to the server.
		Datagram dg = BeginMessage( NETMSG_CMD );
		dg.add_string( full_cmd );
		_client->send_datagram( dg );
	}
}

void ClientBase::start_game( const std::string &mapname )
{
	// Start a local host game

	// Start the server process
	FILE *hserver = _popen( std::string( "p3d_server.exe +map " + mapname ).c_str(), "wt" );
	if ( hserver == NULL )
	{
		nassert_raise( "Could not start server process!" );
		return;
	}

	// Connect the client to the server
	NetAddress naddr;
	naddr.set_host( "127.0.0.1", 27015 );
	_client->connect( naddr );
}

void ClientBase::cleanup_bsp_level()
{
}

void ClientBase::load_bsp_level( const Filename &path, bool is_transition )
{
	_client->set_client_state( CLIENTSTATE_LOADING );

	HostBase::load_bsp_level( path, is_transition );
	_bsp_loader->do_optimizations();
	NodePathCollection props = _bsp_level.find_all_matches( "**/+BSPProp" );
	for ( int i = 0; i < props.get_num_paths(); i++ )
	{
		create_and_attach_bullet_nodes( props[i] );
	}

	_bsp_level.reparent_to( _render );

	_client->set_client_state( CLIENTSTATE_PLAYING );
}

void ClientBase::adjust_window_aspect_ratio( float ratio )
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

void ClientBase::window_event()
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

void ClientBase::handle_window_event( const Event *e, void *data )
{
	ClientBase *self = (ClientBase *)data;
	self->window_event();
}

void ClientBase::do_frame()
{	
	HostBase::do_frame();
}

bool ClientBase::startup()
{
	if ( !HostBase::startup() )
	{
		return false;
	}

	setup_input();

	// Set default mappings
	_key_mappings->add_mapping( IN_FORWARD, KeyboardButton::ascii_key( 'w' ) );
	_key_mappings->add_mapping( IN_BACK, KeyboardButton::ascii_key( 's' ) );
	_key_mappings->add_mapping( IN_MOVELEFT, KeyboardButton::ascii_key( 'a' ) );
	_key_mappings->add_mapping( IN_MOVERIGHT, KeyboardButton::ascii_key( 'd' ) );
	_key_mappings->add_mapping( IN_WALK, KeyboardButton::alt() );
	_key_mappings->add_mapping( IN_SPEED, KeyboardButton::shift() );
	_key_mappings->add_mapping( IN_JUMP, KeyboardButton::space() );
	_key_mappings->add_mapping( IN_DUCK, KeyboardButton::control() );
	_key_mappings->add_mapping( IN_RELOAD, KeyboardButton::ascii_key( 'r' ) );
	_key_mappings->add_mapping( IN_ATTACK, MouseButton::one() );
	_key_mappings->add_mapping( IN_ATTACK2, MouseButton::three() );

	TextNode::set_default_font( new DynamicTextFont( "models/fonts/cour.ttf" ) );

	if ( cl_showfps )
	{
		_fps_meter->enable();
	}

	_event_mgr->add_hook( "window-event", handle_window_event, this );

	return true;
}

void ClientBase::shutdown()
{
	_event_mgr->remove_hook( "window-event", handle_window_event, this );

	HostBase::shutdown();

	_fps_meter->disable();
	_fps_meter = nullptr;

	_audio3d = nullptr;
	_music_manager->shutdown();
	_sfx_manager->shutdown();
	_music_manager = nullptr;
	_sfx_manager = nullptr;

	_render_task->remove();
	_render_task = nullptr;
	_data_task->remove();
	_data_task = nullptr;
	_audio_task->remove();
	_audio_task = nullptr;

	_post_process->shutdown();
	_post_process = nullptr;

	_graphics_window->remove_all_display_regions();
	_graphics_engine->remove_all_windows();
	_graphics_window = nullptr;
	_gsg = nullptr;
	_graphics_pipe = nullptr;

	_mouse_and_keyboard.clear();
	_mouse_watcher.clear();
	_kb_button_thrower.clear();

	_dataroot.remove_node();

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
}

void ClientBase::do_net_tick()
{
	_client->tick();
}

void ClientBase::do_game_tick()
{
	_task_manager->poll();
}

void ClientBase::setup_dgraph()
{
	//
	// Data graph
	//

	_dataroot = NodePath( "dataroot" );

	for ( int i = 0; i < _graphics_window->get_num_input_devices(); i++ )
	{
		std::ostringstream ss;
		ss << "MouseAndKeyboard " << i;
		NodePath mak = _dataroot.attach_new_node( new MouseAndKeyboard( _graphics_window, i, ss.str() ) );
		_mouse_and_keyboard.add_path( mak );

		// Watch the mouse
		PT( MouseWatcher ) mw = new MouseWatcher( "watcher" );
		ModifierButtons mmods = mw->get_modifier_buttons();
		mmods.add_button( KeyboardButton::shift() );
		mmods.add_button( KeyboardButton::control() );
		mmods.add_button( KeyboardButton::alt() );
		mmods.add_button( KeyboardButton::meta() );
		mw->set_modifier_buttons( mmods );
		NodePath mwnp = mak.attach_new_node( mw );
		_mouse_watcher.add_path( mwnp );

		// Watch for keyboard buttons
		PT( ButtonThrower ) bt = new ButtonThrower( "kb-events" );
		ModifierButtons mods;
		mods.add_button( KeyboardButton::shift() );
		mods.add_button( KeyboardButton::control() );
		mods.add_button( KeyboardButton::alt() );
		mods.add_button( KeyboardButton::meta() );
		bt->set_modifier_buttons( mods );
		NodePath btnp = mak.attach_new_node( bt );
		_kb_button_thrower.add_path( btnp );
	}
}

void ClientBase::setup_shaders()
{
	//
	// Shaders
	//

	_shader_generator = new BSPShaderGenerator( _graphics_window, _gsg, _camera, _render );
	_gsg->set_shader_generator( _shader_generator );
	_shader_generator->start_update();
	_shader_generator->add_shader( new VertexLitGenericSpec );
	_shader_generator->add_shader( new UnlitGenericSpec );
	_shader_generator->add_shader( new UnlitNoMatSpec );
	_shader_generator->add_shader( new LightmappedGenericSpec );
	_shader_generator->add_shader( new CSMRenderSpec );
	_shader_generator->add_shader( new SkyBoxSpec );
	_shader_generator->add_shader( new DecalModulateSpec );

	setup_postprocess();
}

void ClientBase::setup_postprocess()
{
	_post_process = new GamePostProcess;
	_post_process->startup( _graphics_window );
	_post_process->add_camera( _cam );

	_post_process->_hdr_enabled = ConfigVariableBool( "mat_hdr", true );
	_post_process->_bloom_enabled = ConfigVariableBool( "mat_bloom", true );
	_post_process->_fxaa_enabled = ConfigVariableBool( "mat_fxaa", true );

	if ( _post_process->_hdr_enabled )
		_render.set_attrib( LightRampAttrib::make_identity() );
	else
		_render.set_attrib( LightRampAttrib::make_default() );

	_post_process->setup();
}

void ClientBase::make_bsp()
{
	_bsp_loader = new CL_BSPLoader;
	BSPLoader::set_global_ptr( _bsp_loader );
}

void ClientBase::setup_bsp()
{
	_bsp_loader->set_gamma( 2.2f );
	_bsp_loader->set_render( _render );
	_bsp_loader->set_want_visibility( true );
	_bsp_loader->set_want_lightmaps( true );
	_bsp_loader->set_visualize_leafs( false );
	_bsp_loader->set_physics_world( _physics_world );
	_bsp_loader->set_win( _graphics_window );
	_bsp_loader->set_camera( _camera );
}

void ClientBase::setup_scene()
{
	HostBase::setup_scene();

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
		return;
	}
}

void ClientBase::setup_camera()
{
	//
	// Camera
	//

	_camera = NodePath( new ModelRoot( "camera" ) );
	DCAST( ModelRoot, _camera.node() )->set_preserve_transform( ModelRoot::PT_local );
	_camera.reparent_to( _render );
	PT( DisplayRegion ) region = _graphics_window->make_display_region();
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
}

void ClientBase::setup_rendering()
{
	//
	// Rendering
	//

	_graphics_pipe = _graphics_pipe_selection->make_default_pipe();
	if ( !_graphics_pipe )
	{
		nassert_raise( "Could not create a graphics pipe!" );
		return;
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
		return;
	}

	_graphics_engine->open_windows();

	_graphics_window->set_clear_color_active( false );
	_graphics_window->set_clear_stencil_active( false );
	_graphics_window->set_clear_depth_active( true );

	_gsg = _graphics_window->get_gsg();
	nassertv( _gsg != nullptr );

	std::cout << "Graphics Information:\n";
	std::cout << "\tVendor: " << _gsg->get_driver_vendor() << "\n";
	std::cout << "\tRenderer: " << _gsg->get_driver_renderer() << "\n";
	std::cout << "\tVersion: " << _gsg->get_driver_version() << "\n";
}

void ClientBase::setup_tasks()
{
	HostBase::setup_tasks();

	_data_task = new GenericAsyncTask( "dataLoop", data_task, this );
	_data_task->set_sort( -50 );
	_task_manager->add( _data_task );

	_client_task = new GenericAsyncTask( "clientLoop", client_task, this );
	_client_task->set_sort( -1 );
	_task_manager->add( _client_task );

	_entities_task = new GenericAsyncTask( "entityLoop", entities_task, this );
	_entities_task->set_sort( 0 );
	_task_manager->add( _entities_task );

	_render_task = new GenericAsyncTask( "igLoop", render_task, this );
	_render_task->set_sort( 50 );
	_task_manager->add( _render_task );

	_audio_task = new GenericAsyncTask( "audioLoop", audio_task, this );
	_audio_task->set_sort( 60 );
	_task_manager->add( _audio_task );

	_cmd_task = new GenericAsyncTask( "commandTask", cmd_task, this );
	_cmd_task->set_sort( 61 );
	_task_manager->add( _cmd_task );
}

void ClientBase::setup_audio()
{
	//
	// Audio
	//

	_sfx_manager = AudioManager::create_AudioManager();
	_music_manager = AudioManager::create_AudioManager();
	_music_manager->set_concurrent_sound_limit( 1 );
	_audio3d = new Audio3DManager( _sfx_manager, _camera, _render, 40 );
	_audio3d->set_drop_off_factor( 0.15f );
	_audio3d->set_doppler_factor( 0.15f );
}

void ClientBase::setup_input()
{
	_input = new CInput;
}

void ClientBase::load_cfg_files()
{
	HostBase::load_cfg_files();
	load_cfg_file( "cfg/client.cfg" );
}

DEFINE_TASK_FUNC( ClientBase::client_task )
{
	ClientBase *self = (ClientBase *)data;
	self->_client->tick();
	return AsyncTask::DS_cont;
}

DEFINE_TASK_FUNC( ClientBase::render_task )
{
	ClientBase *self = (ClientBase *)data;
	self->_graphics_engine->render_frame();
	return AsyncTask::DS_cont;
}

DEFINE_TASK_FUNC( ClientBase::data_task )
{
	ClientBase *self = (ClientBase *)data;
	self->_dgtrav.traverse( self->_dataroot.node() );
	return AsyncTask::DS_cont;
}

DEFINE_TASK_FUNC( ClientBase::audio_task )
{
	ClientBase *self = (ClientBase *)data;
	self->_sfx_manager->update();
	self->_music_manager->update();
	return AsyncTask::DS_cont;
}

DEFINE_TASK_FUNC( ClientBase::entities_task )
{
	ClientBase *self = (ClientBase *)data;

	if ( !self->_client->_connected )
		return AsyncTask::DS_cont;
	
	C_BaseEntity::interpolate_entities();

	for ( size_t i = 0; i < self->_client->_entlist.size(); i++ )
	{
		self->_client->_entlist[i]->run_thinks();
	}

	return AsyncTask::DS_cont;
}

DEFINE_TASK_FUNC( ClientBase::cmd_task )
{
	ClientBase *self = (ClientBase *)data;
	
	self->_client->cmd_tick();

	return AsyncTask::DS_cont;
}