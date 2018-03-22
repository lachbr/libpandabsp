#include "distributedsmoothnode.h"
#include "pp_globals.h"
#include "clientrepository.h"
#include "timemanager.h"
#include "clockdelta.h"

#include <clockObject.h>

ConfigVariableDouble max_future( "smooth-max-future", 0.2 );
ConfigVariableDouble min_suggest_resync( "smooth-min-suggest-resync", 15 );
ConfigVariableBool   enable_smoothing( "smooth-enable-smoothing", true );
ConfigVariableBool   enable_prediction( "smooth-enable-prediction", true );
ConfigVariableDouble lag( "smooth-lag", 0.2 );
ConfigVariableDouble prediction_lag( "smooth-prediction-lag", 0.0 );

NotifyCategoryDef( distributedSmoothNode, "" );

DClassDef( DistributedSmoothNode );

DistributedSmoothNode::DistributedSmoothNode() :
        DistributedNode()
{
        _snbase = new DistributedSmoothNodeBase;
        _smooth_started = false;
        _local_control = false;
        _stopped = false;
}

DistributedSmoothNode::~DistributedSmoothNode()
{
        delete _snbase;
}

DistributedSmoothNodeBase *DistributedSmoothNode::get_snbase() const
{
        return _snbase;
}

void DistributedSmoothNode::generate()
{
        _smoother = SmoothMover();
        _smooth_started = false;
        _last_suggest_resync = 0.0;
        _smooth_wrt_reparents = false;

        DistributedNode::generate();
        _snbase->generate();

        _snbase->set_repository( g_cr, false, 0 );

        activate_smoothing( true, false );

        _stopped = false;
}

void DistributedSmoothNode::disable()
{
        _snbase->disable();
        DistributedNode::disable();
}

void DistributedSmoothNode::delete_do()
{
        DistributedNode::delete_do();
}

void DistributedSmoothNode::smooth_position()
{
        _smoother.compute_and_apply_smooth_pos_hpr( *this, *this );
}

AsyncTask::DoneStatus DistributedSmoothNode::do_smooth_task( GenericAsyncTask *pTask, void *pData )
{
        ( (DistributedSmoothNode *)pData )->smooth_position();
        return AsyncTask::DS_cont;
}

bool DistributedSmoothNode::wants_smoothing() const
{
        return true;
}

void DistributedSmoothNode::start_smooth()
{
        if ( !wants_smoothing() || is_disabled() || is_local() )
        {
                return;
        }

        if ( !_smooth_started )
        {
                string strTaskName = task_name( "smooth" );
                g_base->stop_task( _smooth_task );
                reload_position();
                _smooth_task = g_base->start_task( do_smooth_task, this, strTaskName );
                _smooth_started = true;
        }
}

void DistributedSmoothNode::stop_smooth()
{
        if ( _smooth_started )
        {
                g_base->stop_task( _smooth_task );
                force_to_true_position();
                _smooth_started = false;
        }
}

void DistributedSmoothNode::set_smooth_wrt_reparents( bool bFlag )
{
        _smooth_wrt_reparents = bFlag;
}

bool DistributedSmoothNode::get_smooth_wrt_reparents() const
{
        return _smooth_wrt_reparents;
}

void DistributedSmoothNode::force_to_true_position()
{
        if ( !is_local() && _smoother.get_latest_position() )
        {
                _smoother.apply_smooth_pos_hpr( *this, *this );
        }
        _smoother.clear_positions( 1 );
}

void DistributedSmoothNode::reload_position()
{
        _smoother.clear_positions( 0 );
        _smoother.set_pos_hpr( get_pos(), get_hpr() );
        _smoother.set_phony_timestamp();
        _smoother.mark_position();
}

void DistributedSmoothNode::check_resume( int timestamp )
{
        if ( _stopped )
        {
                double curr_time = g_global_clock->get_frame_time();
                double now = curr_time - _smoother.get_expected_broadcast_period();
                double last = _smoother.get_most_recent_timestamp();
                if ( now > last )
                {
                        double local = ClockDelta::get_global_ptr()->network_to_local_time(
                                timestamp, curr_time );

                        _smoother.set_phony_timestamp( local, true );
                        _smoother.mark_position();
                }
        }

        _stopped = false;
}

void DistributedSmoothNode::set_sm_stop( int timestamp )
{
        set_component_t_live( timestamp );
        _stopped = true;
}

void DistributedSmoothNode::set_sm_stop_field( DCFuncArgs )
{
        ( (DistributedSmoothNode *)data )->set_sm_stop( packer.unpack_int() );
}

void DistributedSmoothNode::set_sm_pos( float x, float y, float z, int timestamp )
{
        check_resume( timestamp );
        _smoother.set_x( x );
        _smoother.set_y( y );
        _smoother.set_z( z );
        set_component_t_live( timestamp );
}

void DistributedSmoothNode::set_sm_pos_field( DCFuncArgs )
{
        DistributedSmoothNode *obj = (DistributedSmoothNode *)data;
        float x = packer.unpack_double();
        float y = packer.unpack_double();
        float z = packer.unpack_double();
        int ts = packer.unpack_int();
        obj->set_sm_pos( x, y, z, ts );
}

void DistributedSmoothNode::get_sm_pos_field( DCFuncArgs )
{
        DistributedSmoothNode *obj = (DistributedSmoothNode *)data;
        packer.pack_double( obj->get_x() );
        packer.pack_double( obj->get_y() );
        packer.pack_double( obj->get_z() );
        packer.pack_int( 0 );
}

void DistributedSmoothNode::set_sm_hpr( float h, float p, float r, int timestamp )
{
        check_resume( timestamp );
        _smoother.set_h( h );
        _smoother.set_p( p );
        _smoother.set_r( r );
        set_component_t_live( timestamp );
}

void DistributedSmoothNode::set_sm_hpr_field( DCFuncArgs )
{
        float h = packer.unpack_double();
        float p = packer.unpack_double();
        float r = packer.unpack_double();
        int ts = packer.unpack_int();
        ( (DistributedSmoothNode *)data )->set_sm_hpr( h, p, r, ts );
}

void DistributedSmoothNode::get_sm_hpr_field( DCFuncArgs )
{
        DistributedSmoothNode *obj = (DistributedSmoothNode *)data;
        packer.pack_double( obj->get_h() );
        packer.pack_double( obj->get_p() );
        packer.pack_double( obj->get_r() );
        packer.pack_int( 0 );
}

void DistributedSmoothNode::set_curr_l_field( DCFuncArgs )
{
        DistributedSmoothNode *obj = (DistributedSmoothNode *)data;

        CHANNEL_TYPE l = packer.unpack_uint64();
        int ts = packer.unpack_int();
        obj->_snbase->set_curr_l( l );
}

void DistributedSmoothNode::get_curr_l_field( DCFuncArgs )
{
        DistributedSmoothNode *obj = (DistributedSmoothNode *)data;
        packer.pack_uint64( obj->get_zone_id() );
        packer.pack_int( 0 );
}

void DistributedSmoothNode::clear_smoothing()
{
        _smoother.clear_positions( 1 );
}

void DistributedSmoothNode::b_clear_smoothing()
{
        _snbase->d_clear_smoothing();
        clear_smoothing();
}

void DistributedSmoothNode::clear_smoothing_field( DCFuncArgs )
{
        ( (DistributedSmoothNode *)data )->clear_smoothing();
}

void DistributedSmoothNode::wrt_reparent_to( NodePath &parent )
{
        if ( _smooth_started )
        {
                if ( _smooth_wrt_reparents )
                {
                        _smoother.handle_wrt_reparent( get_parent(), parent );
                        NodePath::wrt_reparent_to( parent );
                }
                else
                {
                        force_to_true_position();
                        NodePath::wrt_reparent_to( parent );
                        reload_position();
                }
        }
        else
        {
                NodePath::wrt_reparent_to( parent );
        }
}

void DistributedSmoothNode::d_set_parent( int parent_token )
{
        DistributedNode::d_set_parent( parent_token );

        force_to_true_position();
        _snbase->send_current_position();
}

void DistributedSmoothNode::d_suggest_resync( DOID_TYPE av_id, int timestamp_a, int timestamp_b,
                                              double server_time, float uncertainty )
{
        double server_time_sec = cfloor( server_time );
        double server_time_usec = ( server_time - server_time_sec ) * 10000.0;

        BeginCLUpdate( "suggest_resync" );
        AddArg( uint, av_id );
        AddArg( int, timestamp_a );
        AddArg( int, timestamp_b );
        AddArg( int, server_time_sec );
        AddArg( uint, server_time_usec );
        AddArg( double, uncertainty );
        SendCLUpdate( "suggest_resync" );
}

void DistributedSmoothNode::suggest_resync( DOID_TYPE av_id, int timestamp_a, int timestamp_b,
                                            int server_time_sec, uint16_t server_time_usec, float uncertainty )
{
        double server_time = ( server_time_sec + server_time_usec ) / 10000.0;
        bool result = p2p_resync( av_id, timestamp_a, server_time, uncertainty );
        if ( ClockDelta::get_global_ptr()->get_uncertainty() > -1.0 )
        {
                DistributedSmoothNode *other = (DistributedSmoothNode *)g_cr->get_do( av_id );
                if ( other == nullptr )
                {
                        distributedSmoothNode_cat.info()
                                << "Warning: couldn't find the avatar " << av_id << "\n";
                }
                else
                {
                        double real_time = g_global_clock->get_real_time();
                        server_time = real_time - ClockDelta::get_global_ptr()->get_delta();
                        distributedSmoothNode_cat.info()
                                << "Returning resync for " << _do_id << "; local time is " << real_time
                                << " and server time is " << server_time << ".\n";
                        other->d_return_resync( g_cr->get_local_avatar_do_id(), timestamp_b,
                                                server_time,
                                                ClockDelta::get_global_ptr()->get_uncertainty() );
                }
        }
}

void DistributedSmoothNode::suggest_resync_field( DCFuncArgs )
{
        DOID_TYPE av_id = packer.unpack_uint();
        int timestamp_a = packer.unpack_int();
        int timestamp_b = packer.unpack_int();
        int server_time_sec = packer.unpack_int();
        uint16_t server_time_usec = packer.unpack_uint();
        double uncertainty = packer.unpack_double();

        ( (DistributedSmoothNode *)data )->suggest_resync( av_id, timestamp_a, timestamp_b,
                                                           server_time_sec, server_time_usec,
                                                           uncertainty );
}

void DistributedSmoothNode::d_return_resync( DOID_TYPE av_id, int timestamp_b, double server_time, float uncertainty )
{
        double server_time_sec = cfloor( server_time );
        double server_time_usec = ( server_time - server_time_sec ) * 10000.0;

        BeginCLUpdate( "return_resync" );
        AddArg( uint, av_id );
        AddArg( int, timestamp_b );
        AddArg( int, server_time_sec );
        AddArg( uint, server_time_usec );
        AddArg( double, uncertainty );
        SendCLUpdate( "return_resync" );
}

void DistributedSmoothNode::return_resync( DOID_TYPE av_id, int timestamp_b, int server_time_sec, uint16_t server_time_usec,
                                           float uncertainty )
{
        double server_time = ( server_time_sec + server_time_usec ) / 10000.0;
        p2p_resync( av_id, timestamp_b, server_time, uncertainty );
}

void DistributedSmoothNode::return_resync_field( DCFuncArgs )
{
        DOID_TYPE av_id = packer.unpack_uint();
        int timestamp_b = packer.unpack_int();
        int server_time_sec = packer.unpack_int();
        uint16_t server_time_usec = packer.unpack_uint();
        double uncertainty = packer.unpack_double();

        ( (DistributedSmoothNode *)data )->return_resync( av_id, timestamp_b, server_time_sec,
                                                          server_time_usec, uncertainty );
}

bool DistributedSmoothNode::p2p_resync( DOID_TYPE av_id, int timestamp, double server_time, float uncertainty )
{
        bool got_sync = ClockDelta::get_global_ptr()->p2p_resync( av_id, timestamp, server_time, uncertainty );

        if ( !got_sync )
        {
                if ( g_cr->get_time_manager() != nullptr )
                {
                        stringstream ss;
                        ss << "suggested by " << av_id;
                        g_cr->get_time_manager()->synchronize( ss.str() );
                }
        }

        return got_sync;
}

void DistributedSmoothNode::activate_smoothing( bool smoothing, bool prediction )
{
        if ( smoothing )
        {
                if ( prediction )
                {
                        // Prediction and smoothing.
                        _smoother.set_smooth_mode( SmoothMover::SM_on );
                        _smoother.set_prediction_mode( SmoothMover::PM_on );
                        _smoother.set_delay( prediction_lag );
                }
                else
                {
                        // Smoothing, but no prediction.
                        _smoother.set_smooth_mode( SmoothMover::SM_on );
                        _smoother.set_prediction_mode( SmoothMover::PM_off );
                        _smoother.set_delay( lag );
                }
        }
        else
        {
                // No smoothing, no prediction.
                _smoother.set_smooth_mode( SmoothMover::SM_off );
                _smoother.set_prediction_mode( SmoothMover::PM_off );
                _smoother.set_delay( 0.0 );
        }
}

void DistributedSmoothNode::set_component_t( int timestamp )
{
        _smoother.set_phony_timestamp();
        _smoother.clear_positions( 1 );
        _smoother.mark_position();

        force_to_true_position();
}

void DistributedSmoothNode::set_component_t_live( int timestamp )
{
        if ( timestamp == 0 )
        {
                if ( _smoother.has_most_recent_timestamp() )
                {
                        _smoother.set_timestamp( _smoother.get_most_recent_timestamp() );
                }
                else
                {
                        _smoother.set_phony_timestamp();
                }
                _smoother.mark_position();
        }
        else
        {
                double now = g_global_clock->get_frame_time();
                double local = ClockDelta::get_global_ptr()->network_to_local_time( timestamp, now );
                double real_time = g_global_clock->get_real_time();
                double chug = real_time - now;

                double how_far_future = local - now;
                if ( how_far_future - chug >= max_future )
                {
                        if ( ClockDelta::get_global_ptr()->get_uncertainty() != -1 &&
                             real_time - _last_suggest_resync >= min_suggest_resync )
                        {
                                _last_suggest_resync = real_time;
                                int timestamp_b = ClockDelta::get_global_ptr()->local_to_network_time( real_time );
                                double server_time = real_time - ClockDelta::get_global_ptr()->get_delta();
                                distributedSmoothNode_cat.info()
                                        << "Suggesting resync for " << _do_id << ", with discrepency " << how_far_future - chug
                                        << "; local time is " << real_time << " and server time is " << server_time << "." << endl;
                                d_suggest_resync( g_cr->get_local_avatar_do_id(), timestamp,
                                                  timestamp_b, server_time,
                                                  ClockDelta::get_global_ptr()->get_uncertainty() );
                        }
                }

                _smoother.set_timestamp( local );
                _smoother.mark_position();
        }

        if ( !_local_control && !_smooth_started && _smoother.get_latest_position() )
        {
                _smoother.apply_smooth_pos_hpr( *this, *this );
        }
}