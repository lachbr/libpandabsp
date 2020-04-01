#ifndef CLOCKDELTA_H
#define CLOCKDELTA_H

#include "config_game_shared.h"
#include <event.h>

static const int network_time_bits = 16;
static const double network_time_precision = 100.0;

static const int network_time_mask = ( 1 << network_time_bits ) - 1;
static const int network_time_signed_mask = network_time_mask >> 1;
static const int network_time_top_bits = 32 - network_time_precision;

static const double clock_drift_per_hour = 1.0;

static const double clock_drift_per_second = clock_drift_per_hour / 3600.0;

static const double p2p_resync_delay = 10.0;

NotifyCategoryDeclNoExport( clockDelta );

class ClockObject;

class EXPORT_GAME_SHARED CClockDelta
{
public:
	CClockDelta();

	double get_delta() const;
	double get_uncertainty() const;
	double get_last_resync() const;
	double get_latency() const;

	void clear();

	void resynchronize( double localtime, double nettime, double newuncertaincy, double latency, bool trustnew = true );

	bool p2p_resync( uint32_t avid, int timestamp, double servertime, double uncertainty );

	bool new_delta( double localtime, double newdelta, double newuncertainty, bool trust_new = true );

	double get_local_network_time();

	double network_to_local_time( int nettime, double now = -1, int bits = 16, double tickspersec = network_time_precision );

	int local_to_network_time( double localtime, int bits = 16, double tickspersec = network_time_precision );

	int get_real_network_time( int bits = 16, double tickspersec = network_time_precision );

	int get_frame_network_time( int bits = 16, double tickspersec = network_time_precision );

	double local_elapsed_time( int nettime, int bits = 16, double tickspersec = network_time_precision );

	static CClockDelta *get_global_ptr();


private:
	double _delta;
	double _last_resync;
	double _uncertainty;
	double _latency;

	void reset_clock( double timedelta );
	static void reset_clock_event( const Event *e, void *data );

	int sign_extend( int networktime );

	ClockObject *_global_clock;

	static CClockDelta *_global_ptr;
};

#endif // CLOCKDELTA_H