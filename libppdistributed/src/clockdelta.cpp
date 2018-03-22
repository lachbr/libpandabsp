#include "clockdelta.h"
#include "pp_globals.h"

#include <eventHandler.h>

ClockDelta *ClockDelta::_global_ptr = (ClockDelta *)nullptr;

NotifyCategoryDef( clockDelta, "" );

ClockDelta::ClockDelta()
{
        EventHandler::get_global_event_handler()->add_hook( "resetClock", reset_clock_event, this );
}

ClockDelta *ClockDelta::get_global_ptr()
{
        if ( _global_ptr == (ClockDelta *)nullptr )
        {
                _global_ptr = new ClockDelta();
        }

        return _global_ptr;
}

double ClockDelta::get_delta() const
{
        return _delta;
}

double ClockDelta::get_uncertainty() const
{
        if ( _uncertainty == -1 )
        {
                return _uncertainty;
        }

        double now = g_global_clock->get_real_time();
        double elapsed = now - _last_resync;
        return _uncertainty + elapsed * clock_drift_per_second;
}

double ClockDelta::get_last_resync() const
{
        return _last_resync;
}

void ClockDelta::clear()
{
        _delta = 0.0;
        _uncertainty = -1;
        _last_resync = 0.0;
}

void ClockDelta::resynchronize( double localtime, double nettime, double newuncertaincy, bool trustnew )
{
        double nd = localtime - ( nettime / network_time_precision );
        new_delta( localtime, nd, newuncertaincy, trustnew );
}

bool ClockDelta::p2p_resync( uint32_t avid, int timestamp, double servertime, double uncertainty )
{
        double now = g_global_clock->get_real_time();
        if ( now - _last_resync < p2p_resync_delay )
        {
                return false;
        }

        double local = network_to_local_time( timestamp, now );
        double elapsed = now - local;
        double delta = ( local + now ) / 2.0 - servertime;

        bool gotsync = false;
        if ( elapsed <= 0 || elapsed > p2p_resync_delay )
        {
                clockDelta_cat.info()
                        << "Ignoring old request for resync from " << avid << ".\n";
        }
        else
        {
                clockDelta_cat.info()
                        << "Got sync +/- " << uncertainty << " s, elapsed "
                        << elapsed << " s, from " << avid << ".\n";
                delta -= elapsed / 2.0;
                gotsync = new_delta( local, delta, uncertainty, false );
        }

        return gotsync;
}

bool ClockDelta::new_delta( double localtime, double newdelta, double newuncertainty, bool trust_new )
{
        double old_uncert = get_uncertainty();
        if ( old_uncert != -1 )
        {
                clockDelta_cat.info()
                        << "Previous delta at " << _delta << " s, +/- " << old_uncert << " s.\n";
                clockDelta_cat.info()
                        << "New delta at " << newdelta << " s, +/- " << newuncertainty << " s.\n";
                double oldlow = _delta - old_uncert;
                double oldhigh = _delta + old_uncert;
                double newlow = newdelta - newuncertainty;
                double newhigh = newdelta + newuncertainty;

                double low = max( oldlow, newlow );
                double high = min( oldhigh, newhigh );

                if ( low > high )
                {
                        if ( !trust_new )
                        {
                                clockDelta_cat.info()
                                        << "Discarding new delta.\n";
                                return false;
                        }
                        clockDelta_cat.info()
                                << "Discarding previous delta.\n";
                }
                else
                {
                        newdelta = ( low + high ) / 2.0;
                        newuncertainty = ( high - low ) / 2.0;
                        clockDelta_cat.info()
                                << "Intersection at " << newdelta << " s, +/- " << newuncertainty << " s.\n";
                }
        }

        _delta = newdelta;
        _uncertainty = newuncertainty;
        _last_resync = localtime;

        return true;
}

double ClockDelta::network_to_local_time( int nettime, double now, int bits, double tickspersec )
{
        if ( now == -1 )
        {
                now = g_global_clock->get_real_time();
        }

        if ( g_global_clock->get_mode() == ClockObject::M_non_real_time &&
             ConfigVariableBool( "movie-network-time", false ) )
        {
                return now;
        }

        int ntime = (int)cfloor( ( ( now - _delta ) * tickspersec ) + 0.5 );

        int diff;
        if ( bits == 16 )
        {
                diff = sign_extend( nettime - ntime );
        }
        else
        {
                diff = nettime - ntime;
        }

        return now + (double)diff / tickspersec;
}

int ClockDelta::local_to_network_time( double localtime, int bits, double tickspersec )
{
        int ntime = (int)cfloor( ( ( localtime - _delta ) * tickspersec ) + 0.5 );
        if ( bits == 16 )
        {
                return sign_extend( ntime );
        }
        return ntime;
}

int ClockDelta::get_real_network_time( int bits, double tickspersec )
{
        return local_to_network_time( g_global_clock->get_real_time(),
                                      bits, tickspersec );
}

int ClockDelta::get_frame_network_time( int bits, double tickspersec )
{
        return local_to_network_time( g_global_clock->get_frame_time(),
                                      bits, tickspersec );
}

double ClockDelta::local_elapsed_time( int nettime, int bits, double tickspersec )
{
        double now = g_global_clock->get_frame_time();
        double dt = now - network_to_local_time( nettime, now, bits, tickspersec );
        return max( dt, 0.0 );
}

int ClockDelta::sign_extend( int nettime )
{
        int r = ( ( nettime + network_time_signed_mask ) & network_time_mask ) - network_time_signed_mask;
        nassertr( r >= -network_time_signed_mask, nettime );
        nassertr( r <= network_time_signed_mask, nettime );
        return r;
}

void ClockDelta::reset_clock( double timedelta )
{
        _delta += timedelta;
}

void ClockDelta::reset_clock_event( const Event *e, void *data )
{
        ( (ClockDelta *)data )->reset_clock( e->get_parameter( 0 ).get_double_value() );
}