#include "frame_rate_meter.h"
#include "clientbase.h"

#include <asyncTaskManager.h>

static const LColor good_color( 0, 1, 0, 1 );
static const LColor medium_color( 1, 0.5f, 0, 1 );
static const LColor bad_color( 1, 0, 0, 1 );

static const double good_fps = 60.0;
static const double medium_fps = 24.0;

static const int fps_meter_sort = 100000;

CFrameRateMeter::CFrameRateMeter() :
	_task( new GenericAsyncTask( "frameRateMeter", update_task, this ) ),
	_tn( nullptr ),
	_last_update_time( 0.0 )
{
}

void CFrameRateMeter::enable()
{
	disable();

	_tn = new TextNode( "frameRateMeter" );
	_tn->set_align( TextNode::A_right );
	_tn->set_text_color( good_color );
	_tn->set_text_scale( 0.04f );

	_tnnp = NodePath( _tn );
	_tnnp.reparent_to( cl->_a2d_top_right, fps_meter_sort );
	_tnnp.set_z( -0.04f );
	_tnnp.set_x( -0.02f );

	AsyncTaskManager::get_global_ptr()->add( _task );
}

void CFrameRateMeter::disable()
{
	if ( !_tnnp.is_empty() )
		_tnnp.remove_node();
	_tn = nullptr;
	
	AsyncTaskManager *atm = AsyncTaskManager::get_global_ptr();
	if ( atm->has_task( _task ) )
		atm->remove( _task );
}

void CFrameRateMeter::update()
{
	ClockObject *clock = ClockObject::get_global_clock();
	double now = clock->get_frame_time();
	int frame_rate = (int)round( 1.0 / ( now - _last_update_time ) );

	char text[10];
	sprintf( text, "%i fps", frame_rate );
	_tn->set_text( text );

	if ( frame_rate >= good_fps )
	{
		_tn->set_text_color( good_color );
	}
	else if ( frame_rate >= medium_fps )
	{
		_tn->set_text_color( medium_color );
	}
	else
	{
		_tn->set_text_color( bad_color );
	}

	_last_update_time = now;
}

AsyncTask::DoneStatus CFrameRateMeter::update_task( GenericAsyncTask *task, void *data )
{
	CFrameRateMeter *self = (CFrameRateMeter *)data;
	self->update();

	return AsyncTask::DS_cont;
}