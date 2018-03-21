#ifndef __PP_BASE_H__
#define __PP_BASE_H__

#include <pandaFramework.h>
#include <nodePath.h>
#include <asyncTask.h>
#include <genericAsyncTask.h>
#include <collisionTraverser.h>
#include <audioManager.h>

#include "config_pandaplus.h"
#include "audio3dmanager.h"

NotifyCategoryDeclNoExport( ppBase );

// Initializes global variables, runs updates on global objects.
class EXPCL_PANDAPLUS PPBase
{
public:
        PPBase( int argc, char *argv[] );

        PandaFramework _framework;
        WindowFramework *_window;

        NodePath _render;
        NodePath _hidden;
        NodePath _aspect2d;
        NodePath _render2d;
        NodePath _camera;

        Audio3DManager _audio3d;

        PT( AudioManager ) _sfx_mgr;
        PT( AudioManager ) _music_mgr;

        PT( AsyncTaskManager ) _task_mgr;

        CollisionTraverser _ctrav;
        CollisionTraverser _shadow_trav;

        //RenderPipeline _rpipeline;

        void define_key( const string &event, const string &description, void( *function )( const Event *, void * ), void *data );
        void undefine_key( const string &event );

        void load_map( const string &file );
        void unload_map();

        int get_resolution_x() const;
        int get_resolution_y() const;

        void set_sleep( PN_stdfloat sleep );
        PN_stdfloat get_sleep() const;

        PT( AudioSound ) load_music( const string &path );
        PT( AudioSound ) load_sfx( const string &path );
        NodePath load_model( const string &path );
        PT( Texture ) load_texture( const string &path );
        PT( TextFont ) load_font( const string &path );

        // Create a task and run it now.
        PT( GenericAsyncTask ) start_task( GenericAsyncTask::TaskFunc func, void *data, const string &name );

        // Create a task and run it after the specified delay.
        PT( GenericAsyncTask ) do_task_later( PN_stdfloat delay, GenericAsyncTask::TaskFunc func, void *data, const string &name );

        // Stop a currently running task.
        void stop_task( AsyncTask *task );

        void set_camera_fov( PN_stdfloat fov );
        void set_camera_near( PN_stdfloat val );
        void set_camera_far( PN_stdfloat val );

        // Change the background color of the window.
        void set_background_color( PN_stdfloat r, PN_stdfloat g, PN_stdfloat b );

        void enable_music();
        void disable_music();
        void enable_sfx();
        void disable_sfx();

        void set_music_volume( PN_stdfloat vol );
        void set_sfx_volume( PN_stdfloat vol );

        // Close all windows and exits the application.
        void shutdown( int exit_code );

        // Apply an Orthographic lens to all 3-d cameras.
        // (useful for 2-d games)
        void use_ortho_lens();

        // Apply a Perspective lens to all 3-d cameras.
        void use_persp_lens();

        // Enable the free movement of the camera through the mouse.
        void enable_mouse();

        // Disable the free movement of the camera through the mouse.
        void disable_mouse();

        void set_mouse_cursor_visible( bool flag );

        // Sets whether or not the music and sound effects should be disabled when the window is minimized.
        void set_stop_audio_on_minimize( bool sfx = true, bool mus = true );

        void mount_multifile( const string &mfpath );

        void run();

        void play_music( PT( AudioSound ) music, bool looping = false, PN_stdfloat volume = 1.0, bool interrupt = true );

private:
        static AsyncTask::DoneStatus tick_task( GenericAsyncTask *task, void *data );

        bool _min_stop_sfx;
        bool _min_stop_mus;

        PN_stdfloat _last_mus_t;

        static AsyncTask::DoneStatus sleep_cycle_task( GenericAsyncTask *task, void *data );

        PT( GenericAsyncTask ) _tick_task;

        PT( GenericAsyncTask ) _sleep_task;

        PN_stdfloat _sleep_time;

        PT( AudioSound ) _current_music;

        static void handle_window_event( const Event *e, void *data );
};

#endif // __PP_BASE_H__