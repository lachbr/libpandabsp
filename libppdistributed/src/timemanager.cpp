#include "timemanager.h"
#include "clockdelta.h"
#include "clientrepository.h"
#include "pp_globals.h"

#include <throw_event.h>
#include <eventHandler.h>

DClassDef( TimeManager );

NotifyCategoryDef( timeManager, "" );

TimeManager::TimeManager()
{
        _this_context = -1;
        _next_context = 0;
        _attempt_count = 0;
        _start = 0;
        _last_attempt = -timemanager_min_wait * 2;
}

void TimeManager::generate()
{
        DistributedObject::generate();

        EventHandler::get_global_event_handler()->add_hook( "clock_error",
                                                            handle_clock_error_event,
                                                            this );

        if ( timemanager_update_freq > 0 )
        {
                start_task();
        }
}

void TimeManager::announce_generate()
{
        DistributedObject::announce_generate();
        g_cr->set_time_manager( this );
        synchronize( "TimeManager::anounce_generate" );
}

void TimeManager::disable()
{
        EventHandler::get_global_event_handler()->remove_hook( "clock_error", handle_clock_error_event, this );
        stop_task();
        _update_task->remove();
        if ( g_cr->get_time_manager() == this )
        {
                g_cr->set_time_manager( nullptr );
        }
        DistributedObject::disable();
}

void TimeManager::delete_do()
{
        DistributedObject::delete_do();
}

void TimeManager::start_task()
{
        stop_task();
        _update_task = new GenericAsyncTask( "timeMgrTask", do_update_task, this );
        _update_task->set_delay( timemanager_update_freq );
        g_task_mgr->add( _update_task );
}

void TimeManager::stop_task()
{
        if ( _update_task != nullptr )
        {
                _update_task->remove();
        }
}

AsyncTask::DoneStatus TimeManager::do_update_task( GenericAsyncTask *task, void *data )
{
        TimeManager *tm = (TimeManager *)data;
        tm->synchronize( "timer" );

        task->set_delay( timemanager_update_freq );

        return AsyncTask::DS_again;
}

void TimeManager::handle_clock_error_event( const Event *e, void *data )
{
        ( (TimeManager *)data )->synchronize( "clock error" );
}

bool TimeManager::synchronize( const string &desc )
{
        double now = g_global_clock->get_real_time();

        double time = now - _last_attempt;
        timeManager_cat.debug()
                << "Time since last sync: " << time << " seconds\n";
        if ( time < timemanager_min_wait )
        {
                return false;
        }

        _task_result = 0;
        _this_context = _next_context;
        _attempt_count = 0;
        _next_context = ( _next_context + 1 ) & 255;
        timeManager_cat.info()
                << "Clock sync: " << desc << "\n";
        _start = now;
        _last_attempt = now;

        d_request_server_time( _this_context );

        return true;
}

void TimeManager::server_time( uint8_t context, int32_t timestamp )
{
        double end = g_global_clock->get_real_time();

        if ( context != _this_context )
        {
                timeManager_cat.info()
                        << "Ignoring TimeManager response for old context " << context << "\n";
                return;
        }

        double elapsed = end - _start;
        _attempt_count++;
        timeManager_cat.info()
                << "Clock sync roundtrip took " << elapsed * 1000.0 << " ms\n";

        double avg = ( _start + end ) / 2.0 - timemanager_extra_skew;
        double uncertainty = ( end - _start ) / 2.0 + abs( timemanager_extra_skew );

        ClockDelta *gcd = ClockDelta::get_global_ptr();

        gcd->resynchronize( avg, timestamp, uncertainty );

        timeManager_cat.info()
                << "Local clock uncertainty +/- " << gcd->get_uncertainty() << " s\n";

        if ( gcd->get_uncertainty() > timemanager_max_uncertainty )
        {
                if ( _attempt_count < timemanager_max_attemps )
                {
                        timeManager_cat.info()
                                << "Uncertainty is too high, trying again.\n";
                        _start = g_global_clock->get_real_time();
                        d_request_server_time( _this_context );
                        return;
                }
                timeManager_cat.info()
                        << "Giving up on uncertainty requirement.\n";
        }

        throw_event( "gotTimeSync" );
        throw_event( unique_name( "gotTimeSync" ) );
}

void TimeManager::server_time_field( DCPacker &packer, void *data )
{
        uint8_t context = packer.unpack_uint();
        int32_t timestamp = packer.unpack_int();
        ( (TimeManager *)data )->server_time( context, timestamp );
}

void TimeManager::d_request_server_time( uint8_t context )
{
        BeginCLUpdate( "request_server_time" );
        AddArg( uint, context );
        SendCLUpdate( "request_server_time" );
}