#include "ppbase.h"
#include "pp_globals.h"
#include "maploader.h"

#include <cIntervalManager.h>
#include <windowProperties.h>
#include <texturePool.h>
#include <fontPool.h>
#include <orthographicLens.h>
#include <perspectiveLens.h>
#include <eventHandler.h>
#include <virtualFileSystem.h>

NotifyCategoryDef( ppBase, "" );

PPBase::PPBase( int argc, char *argv[] )
{
        _framework.open_framework( argc, argv );

        _last_mus_t = 0.0f;
        _min_stop_sfx = false;
        _min_stop_mus = false;
        _current_music = nullptr;

        bool use_win = ConfigVariableString( "window-type", "onscreen" ) != "none";

        if ( use_win )
        {
                _window = _framework.open_window();
                _window->enable_keyboard();
        }


        g_window = _window;
        g_framework = &_framework;

        if ( use_win )
        {
                _render = g_window->get_render();
                _aspect2d = g_window->get_aspect_2d();
                _render2d = g_window->get_render_2d();
                _camera = _window->get_camera_group();
        }

        _hidden = NodePath( "hidden" );

        g_render = _render;
        g_hidden = _hidden;
        g_aspect2d = _aspect2d;
        g_render2d = _render2d;
        g_camera = _camera;

        EventHandler::get_global_event_handler()->add_hook( "window-event", handle_window_event, this );

        _music_mgr = AudioManager::create_AudioManager();
        _music_mgr->set_volume( 0.65 );
        _sfx_mgr = AudioManager::create_AudioManager();

        g_music_mgr = _music_mgr;
        g_sfx_mgr = _sfx_mgr;

        _ctrav = CollisionTraverser( "mainCTrav" );
        _shadow_trav = CollisionTraverser( "shadowTrav" );
        _shadow_trav.set_respect_prev_transform( false );

        _audio3d = Audio3DManager( _sfx_mgr, _camera, _render );
        _audio3d.start_update();
        g_audio3d = &_audio3d;

        _task_mgr = g_task_mgr;

        _tick_task = new GenericAsyncTask( "ppBase_tick", &tick_task, this );
        g_task_mgr->add( _tick_task );

        g_base = this;
}

void PPBase::mount_multifile( const string &mfpath )
{
        VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();
        vfs->mount( mfpath, ".", VirtualFileSystem::MF_read_only );
}

void PPBase::handle_window_event( const Event *e, void *data )
{
        PPBase *obj = (PPBase *)data;
        const WindowProperties props = obj->_window->get_graphics_window()->get_properties();
        if ( !props.get_foreground() || props.get_minimized() )
        {
                // Window is minimized or not in the foreground. Maybe disable the audio.
                if ( obj->_min_stop_mus )
                {
                        if ( obj->_current_music != nullptr )
                        {
                                obj->_last_mus_t = obj->_current_music->get_time();
                        }
                        obj->disable_music();
                }
                if ( obj->_min_stop_sfx )
                {
                        obj->disable_sfx();
                }
        }
        else
        {
                if ( obj->_min_stop_mus )
                {
                        obj->enable_music();
                        if ( obj->_current_music != nullptr )
                        {
                                obj->_current_music->set_time( obj->_last_mus_t );
                        }
                }
                if ( obj->_min_stop_sfx )
                {
                        obj->enable_sfx();
                }
        }
}

void PPBase::shutdown( int exit_code )
{
        ppBase_cat.info()
                << "Exit code: " << exit_code << "\n";

        _framework.close_all_windows();
        _framework.close_framework();
        exit( exit_code );
}

void PPBase::run()
{
        _framework.main_loop();
}

void PPBase::set_sleep( PN_stdfloat sleep )
{
        if ( _sleep_time == sleep )
        {
                return;
        }

        _sleep_time = sleep;
        if ( _sleep_time == 0.0 )
        {
                if ( _sleep_task != nullptr )
                {
                        _sleep_task->remove();
                }
        }
        else
        {
                if ( _sleep_task != nullptr )
                {
                        _sleep_task->remove();
                }
                _sleep_task = new GenericAsyncTask( "clientSleep", sleep_cycle_task, this );
                _sleep_task->set_sort( 55 );
        }
}

AsyncTask::DoneStatus PPBase::sleep_cycle_task( GenericAsyncTask *task, void *data )
{
        PPBase *cls = (PPBase *)data;
        Thread::sleep( cls->_sleep_time );
        return AsyncTask::DS_cont;
}

PN_stdfloat PPBase::get_sleep() const
{
        return _sleep_time;
}

int PPBase::get_resolution_x() const
{
        return _window->get_graphics_window()->get_x_size();
}

int PPBase::get_resolution_y() const
{
        return _window->get_graphics_window()->get_y_size();
}

void PPBase::define_key( const string &name, const string &description, void( *function )( const Event *, void * ), void *data )
{
        _framework.define_key( name, description, function, data );
}

void PPBase::undefine_key( const string &name )
{
        EventHandler::get_global_event_handler()->remove_hooks( name );
}

void PPBase::load_map( const string &file )
{
        unload_map();

        MapLoader::load_map( file );
}

void PPBase::unload_map()
{
        MapLoader::unload_map();
}

AsyncTask::DoneStatus PPBase::tick_task( GenericAsyncTask *task, void *data )
{
        PPBase *base = (PPBase *)data;
        g_base->_music_mgr->update();
        g_base->_sfx_mgr->update();
        if ( ConfigVariableString( "window-type", "onscreen" ) != "none" )
        {
                g_base->_ctrav.traverse( g_base->_render );
                g_base->_shadow_trav.traverse( g_base->_render );
        }

        CIntervalManager::get_global_ptr()->step();

        if ( g_base->_current_music != nullptr )
        {
                if ( g_base->_current_music->status() != AudioSound::PLAYING )
                {
                        // Set the current music to nullptr if the current music is not playing.
                        g_base->_current_music = nullptr;
                }
        }

        return AsyncTask::DS_cont;
}

PT( AudioSound ) PPBase::load_music( const string &path )
{
        return _music_mgr->get_sound( path );
}

PT( AudioSound ) PPBase::load_sfx( const string &path )
{
        return _sfx_mgr->get_sound( path );
}

NodePath PPBase::load_model( const string &path )
{
        return _window->load_model( _framework.get_models(), path );
}

PT( Texture ) PPBase::load_texture( const string &path )
{
        return TexturePool::load_texture( path );
}

PT( TextFont ) PPBase::load_font( const string &path )
{
        return FontPool::load_font( path );
}

void PPBase::set_background_color( PN_stdfloat r, PN_stdfloat g, PN_stdfloat b )
{
        _window->get_display_region_3d()->set_clear_color( LColor( r, g, b, 1.0 ) );
}

void PPBase::enable_music()
{
        _music_mgr->set_active( true );
}

void PPBase::disable_music()
{
        _music_mgr->set_active( false );
}

void PPBase::enable_sfx()
{
        _sfx_mgr->set_active( true );
}

void PPBase::disable_sfx()
{
        _sfx_mgr->set_active( false );
}

void PPBase::set_music_volume( PN_stdfloat vol )
{
        _music_mgr->set_volume( vol );
}

void PPBase::set_sfx_volume( PN_stdfloat vol )
{
        _sfx_mgr->set_volume( vol );
}

void PPBase::enable_mouse()
{
        _window->setup_trackball();
}

void PPBase::disable_mouse()
{
        ppBase_cat.warning()
                << "disable_mouse: not implemented\n";
}

void PPBase::set_mouse_cursor_visible( bool flag )
{
        WindowProperties props;
        props.set_cursor_hidden( !flag );
        _window->get_graphics_window()->request_properties( props );
}

PT( GenericAsyncTask ) PPBase::start_task( GenericAsyncTask::TaskFunc func, void *data, const string &name )
{
        PT( GenericAsyncTask ) task = new GenericAsyncTask( name, func, data );
        _task_mgr->add( task );
        return task;
}

PT( GenericAsyncTask ) PPBase::do_task_later( PN_stdfloat delay, GenericAsyncTask::TaskFunc func, void *data, const string &name )
{
        PT( GenericAsyncTask ) task = new GenericAsyncTask( name, func, data );
        task->set_delay( delay );
        _task_mgr->add( task );
        return task;
}

void PPBase::stop_task( AsyncTask *task )
{
        _task_mgr->remove( task );
}

void PPBase::use_ortho_lens()
{
        int numcams = _window->get_num_cameras();
        for ( int i = 0; i < numcams; i++ )
        {
                Camera *cam = _window->get_camera( i );
                PT( OrthographicLens ) lens = new OrthographicLens();
                cam->set_lens( lens );
        }
}

void PPBase::use_persp_lens()
{
        int numcams = _window->get_num_cameras();
        for ( int i = 0; i < numcams; i++ )
        {
                Camera *cam = _window->get_camera( i );
                PT( PerspectiveLens ) lens = new PerspectiveLens();
                cam->set_lens( lens );
        }
}

void PPBase::set_stop_audio_on_minimize( bool sfx, bool mus )
{
        _min_stop_mus = mus;
        _min_stop_sfx = sfx;
}

void PPBase::play_music( PT( AudioSound ) music, bool looping, PN_stdfloat volume, bool interrupt )
{
        music->set_loop( looping );
        music->set_volume( volume );
        if ( interrupt )
        {
                // Stop the current song if specified.
                if ( _current_music != nullptr )
                {
                        _current_music->stop();
                }
        }
        music->play();
        _current_music = music;
}

void PPBase::set_camera_fov( PN_stdfloat fov )
{
        _window->get_camera( 0 )->get_lens()->set_min_fov( fov / ( 4. / 3. ) );
}

void PPBase::set_camera_near( PN_stdfloat val )
{
        _window->get_camera( 0 )->get_lens()->set_near( val );
}

void PPBase::set_camera_far( PN_stdfloat val )
{
        _window->get_camera( 0 )->get_lens()->set_far( val );
}