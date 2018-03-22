#include "distributedsmoothnode_base.h"
#include "clockdelta.h"
#include "distributedobject_base.h"
#include "pp_globals.h"
#include "clientrepository.h"

#include <genericAsyncTask.h>

static const float smooth_node_epsilon = 0.01;

#define BeginSmUpdate(fieldname)\
DCPacker packer = _dclass->start_client_format_update(fieldname, _do_id);

#define SendSmUpdate(fieldname)\
AddArg(int, ClockDelta::get_global_ptr()->get_real_network_time());\
Datagram dg = _dclass->end_client_format_update(fieldname, packer);\
if (_is_ai)\
{\
        g_air->send(dg);\
}\
else\
{\
        g_cr->send(dg);\
}

#define SendBlankSmUpdate(fieldname)\
DCPacker packer = _dclass->start_client_format_update(fieldname, _do_id);\
Datagram dg = _dclass->end_client_format_update(fieldname, packer);\
if (_is_ai)\
{\
        g_air->send(dg);\
}\
else\
{\
        g_cr->send(dg);\
}


DistributedSmoothNodeBase::DistributedSmoothNodeBase()
{
        _repo = nullptr;
        _is_ai = false;
        _ai_id = 0;
        _curr_l[0] = 0;
        _curr_l[1] = 0;
}

DistributedSmoothNodeBase::~DistributedSmoothNodeBase()
{
}

void DistributedSmoothNodeBase::set_repository( ClientRepository *repo, bool is_ai, CHANNEL_TYPE ai_id )
{
        _repo = repo;
        _is_ai = is_ai;
        _ai_id = ai_id;
}

void DistributedSmoothNodeBase::initialize( const NodePath &np, DCClassPP *dclass, DOID_TYPE do_id )
{
        _np = np;
        _dclass = dclass;
        _do_id = do_id;

        nassertv( !_np.is_empty() );

        _store_xyz = _np.get_pos();
        _store_hpr = _np.get_hpr();
        _store_stop = false;
}

void DistributedSmoothNodeBase::send_everything()
{
        _curr_l[0] = _curr_l[1];
        d_set_sm_pos( _store_xyz[0], _store_xyz[1], _store_xyz[2] );
        d_set_sm_hpr( _store_hpr[0], _store_hpr[1], _store_hpr[2] );
        d_set_curr_l( _curr_l[0] );
}

bool DistributedSmoothNodeBase::only_changed( int flags, int compare )
{
        return ( flags & compare ) != 0 && ( flags & ~compare ) == 0;
}

void DistributedSmoothNodeBase::broadcast_pos_hpr_full()
{
        LPoint3 xyz = _np.get_pos();
        LVecBase3 hpr = _np.get_hpr();

        int flags = 0;

        if ( !IS_THRESHOLD_EQUAL( _store_xyz[0], xyz[0], smooth_node_epsilon ) ||
             !IS_THRESHOLD_EQUAL( _store_xyz[1], xyz[1], smooth_node_epsilon ) ||
             !IS_THRESHOLD_EQUAL( _store_xyz[2], xyz[2], smooth_node_epsilon ) )
        {
                _store_xyz = xyz;
                flags |= F_new_pos;
        }

        if ( !IS_THRESHOLD_EQUAL( _store_hpr[0], hpr[0], smooth_node_epsilon ) ||
             !IS_THRESHOLD_EQUAL( _store_hpr[1], hpr[1], smooth_node_epsilon ) ||
             !IS_THRESHOLD_EQUAL( _store_hpr[2], hpr[2], smooth_node_epsilon ) )
        {
                _store_hpr = hpr;
                flags |= F_new_hpr;
        }

        if ( _curr_l[0] != _curr_l[1] )
        {
                // Location (zoneid) has changed, send out all info copy over 'set'
                // location over to 'sent' location
                _store_stop = false;
                send_everything();
        }
        else if ( flags == 0 )
        {
                if ( !_store_stop )
                {
                        _store_stop = true;
                        d_set_sm_stop();
                }
        }
        else if ( only_changed( flags, F_new_pos ) )
        {
                // Only a change in pos.
                _store_stop = false;
                d_set_sm_pos( _store_xyz[0], _store_xyz[1], _store_xyz[2] );
        }
        else if ( only_changed( flags, F_new_hpr ) )
        {
                // Only a change in hpr.
                _store_stop = false;
                d_set_sm_hpr( _store_hpr[0], _store_hpr[1], _store_hpr[2] );
        }
        else
        {
                // Any other change.
                _store_stop = false;
                send_everything();
        }
}

void DistributedSmoothNodeBase::d_set_curr_l( uint64_t l )
{
        BeginSmUpdate( "set_curr_l" );
        AddArg( uint64, l );
        SendSmUpdate( "set_curr_l" );
}

void DistributedSmoothNodeBase::set_curr_l( uint64_t l )
{
        _curr_l[1] = l;
}

void DistributedSmoothNodeBase::d_set_sm_hpr( float h, float p, float r )
{
        BeginSmUpdate( "set_sm_hpr" );
        AddArg( double, h );
        AddArg( double, p );
        AddArg( double, r );
        SendSmUpdate( "set_sm_hpr" );
}

// SetSmHpr is pure virtual, defined in distributedSmoothNode/AI.cpp

void DistributedSmoothNodeBase::d_set_sm_pos( float x, float y, float z )
{
        BeginSmUpdate( "set_sm_pos" );
        AddArg( double, x );
        AddArg( double, y );
        AddArg( double, z );
        SendSmUpdate( "set_sm_pos" );
}

// SetSmPos is pure virtual, defined in DistributedSmoothNode/AI.cpp

void DistributedSmoothNodeBase::d_set_sm_stop()
{
        BeginSmUpdate( "set_sm_stop" );
        SendSmUpdate( "set_sm_stop" );
}

// SetSmStop is pure virtual, defined in distributedSmoothNode/AI.cpp

void DistributedSmoothNodeBase::generate()
{
}

void DistributedSmoothNodeBase::disable()
{
        stop_pos_hpr_broadcast();
}

void DistributedSmoothNodeBase::d_clear_smoothing()
{
        SendBlankSmUpdate( "clear_smoothing" );
}

// ClearSmoothing is pure virtual, defined in DistributedSmoothNodeBase/AI.cpp

string DistributedSmoothNodeBase::get_pos_hpr_broadcast_task_name() const
{
        stringstream ss;
        ss << "sendPosHpr-" << _do_id;
        return ss.str();
}

void DistributedSmoothNodeBase::set_pos_hpr_broadcast_period( float period )
{
        _broadcast_period = period;
}

float DistributedSmoothNodeBase::get_pos_hpr_broadcast_period() const
{
        return _broadcast_period;
}

void DistributedSmoothNodeBase::stop_pos_hpr_broadcast()
{
        g_base->stop_task( _broadcast_task );
}

bool DistributedSmoothNodeBase::pos_hpr_broadcast_started() const
{
        return true;
}

bool DistributedSmoothNodeBase::want_smooth_broadcast_task() const
{
        return true;
}

void DistributedSmoothNodeBase::start_pos_hpr_broadcast( float period, bool stagger )
{

        string task_name = get_pos_hpr_broadcast_task_name();
        set_pos_hpr_broadcast_period( period );
        send_everything();

        if ( want_smooth_broadcast_task() )
        {
                _broadcast_task = g_base->do_task_later( _broadcast_period, pos_hpr_broadcast_task, this, task_name );
        }
}

AsyncTask::DoneStatus DistributedSmoothNodeBase::pos_hpr_broadcast_task( GenericAsyncTask *task, void *data )
{
        DistributedSmoothNodeBase *obj = (DistributedSmoothNodeBase *)data;
        obj->broadcast_pos_hpr_full();
        task->set_delay( obj->_broadcast_period );
        return AsyncTask::DS_again;
}

void DistributedSmoothNodeBase::send_current_position()
{
        send_everything();
}
