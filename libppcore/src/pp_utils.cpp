#include "pp_utils.h"
#include "pp_globals.h"

#include <virtualFileSystem.h>

SerialNumGen::SerialNumGen( int start )
{
        _counter = start - 1;
}

int SerialNumGen::next()
{
        return ++_counter;
}

FrameDelayedCall::FrameDelayedCall()
{
        _finished = true;
}

FrameDelayedCall::FrameDelayedCall( const string &name, CallbackFunc *callback, void *data, int frames, CancelFunc *cancelfunc )
{
        _name = name;
        _callback = callback;
        _data = data;
        _num_frames = frames;
        _cancel_func = cancelfunc;
        _task_name = PPUtils::unique_name( "FrameDelayedCall-" + _name );
        _finished = true;
        _counter = 0;
        start_task();
}

bool FrameDelayedCall::is_finished() const
{
        return _finished;
}

void FrameDelayedCall::destroy()
{
        _finished = true;
        stop_task();
}

void FrameDelayedCall::finish()
{
        if ( !_finished )
        {
                _finished = true;
                ( *_callback )( _data );
        }
        destroy();
}

void FrameDelayedCall::start_task()
{
        _counter = 0;
        _finished = false;
        _task = new GenericAsyncTask( _task_name, tick_task, this );
        g_task_mgr->add( _task );
}

void FrameDelayedCall::stop_task()
{
        _finished = true;
        _task->remove();
}

AsyncTask::DoneStatus FrameDelayedCall::tick_task( GenericAsyncTask *task, void *data )
{
        FrameDelayedCall *cls = (FrameDelayedCall *)data;
        if ( ( *cls->_cancel_func )( cls->_data ) )
        {
                cls->destroy();
                return AsyncTask::DS_done;
        }
        cls->_counter++;
        if ( cls->_counter >= cls->_num_frames )
        {
                cls->finish();
                return AsyncTask::DS_done;
        }
        return AsyncTask::DS_cont;
}

SerialNumGen PPUtils::serial_gen;

int PPUtils::serial_num()
{
        return serial_gen.next();
}

string PPUtils::unique_name( const string &name )
{
        stringstream ss;
        ss << name << "-" << serial_gen.next();
        return ss.str();
}

void PPUtils::mount_multifile( const string &mfpath )
{
        VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();
        vfs->mount( mfpath, ".", VirtualFileSystem::MF_read_only );
}

PT( CLerpNodePathInterval ) PPUtils::make_nodepath_interval( const string &name, double duration,
                                                             const NodePath &node )
{
        return new CLerpNodePathInterval( name, duration, CLerpInterval::BT_no_blend,
                                          false, true, node, NodePath() );
}

PT( DynamicTextFont ) PPUtils::load_dynamic_font( const Filename &file )
{
        PT( DynamicTextFont ) font = new DynamicTextFont( file );
        font->set_pixels_per_unit( 72 );
        font->set_minfilter( SamplerState::FT_linear_mipmap_linear );
        font->set_magfilter( SamplerState::FT_linear );

        return font;
}