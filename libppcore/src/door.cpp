#include "door.h"

#include "pp_globals.h"

#define DOOR_SPEED_DIVISOR 5.0

TypeHandle Door::_type_handle;

Door::Door( const string &name ) :
        PandaNode( name )
{
        _opened_start_t = 0.0;
}

void Door::set_wait( PN_stdfloat wait )
{
        _wait = wait;
}

PN_stdfloat Door::get_wait() const
{
        return _wait;
}

void Door::set_move_dir( LPoint3f &movedir )
{
        _move_dir = movedir;
}

LPoint3f Door::get_move_dir() const
{
        return _move_dir;
}

void Door::set_speed( PN_stdfloat speed )
{
        _speed = speed;
}

PN_stdfloat Door::get_speed() const
{
        return _speed;
}

void Door::set_spawn_pos( SpawnPos spawnpos )
{
        _spawn_pos = spawnpos;
}

Door::SpawnPos Door::get_spawn_pos() const
{
        return _spawn_pos;
}

void Door::set_locked_sentence( const string &locked_sentence )
{
        _locked_sentence = locked_sentence;
}

string Door::get_locked_sentence() const
{
        return _locked_sentence;
}

void Door::set_unlocked_sentence( const string &unlocked_sentence )
{
        _unlocked_sentence = unlocked_sentence;
}

string Door::get_unlocked_sentence() const
{
        return _unlocked_sentence;
}

void Door::set_door_np( NodePath &np )
{
        _door_np = np;
}

NodePath Door::get_door_np() const
{
        return _door_np;
}

void Door::tick()
{
        if ( _state == DS_closed )
        {
                _door_np.set_pos( 0, 0, 0 );
        }
        else if ( _state == DS_opened )
        {
                _door_np.set_pos( _move_dir );
                if ( g_global_clock->get_real_time() - _opened_start_t >= _wait )
                {
                        _state = DS_closing;
                        _move_ival = new CLerpNodePathInterval( "Door.moveDown", DOOR_SPEED_DIVISOR / _speed,
                                                                CLerpInterval::BT_ease_in_out, true, false, _door_np, NodePath() );
                        _move_ival->set_start_pos( _move_dir );
                        _move_ival->set_end_pos( LVecBase3f( 0.0, 0.0, 0.0 ) );
                        _move_ival->start();
                }
        }
        else if ( _state == DS_opening )
        {
                if ( _move_ival->get_t() >= _move_ival->get_duration() )
                {
                        _state = DS_opened;
                        _move_ival->finish();
                        _opened_start_t = g_global_clock->get_real_time();
                }
        }
        else if ( _state == DS_closing )
        {
                if ( _move_ival->get_t() >= _move_ival->get_duration() )
                {
                        _state = DS_closed;
                        _move_ival->finish();
                }
        }
}

AsyncTask::DoneStatus Door::tick_task( GenericAsyncTask *task, void *data )
{
        ( (Door *)data )->tick();
        return AsyncTask::DS_cont;
}

void Door::start_tick()
{
        _tick_task = new GenericAsyncTask( "DoorTick", tick_task, this );
        g_task_mgr->add( _tick_task );
}

void Door::stop_tick()
{
        if ( _tick_task != (GenericAsyncTask *)nullptr )
        {
                _tick_task->remove();
                _tick_task = (GenericAsyncTask *)nullptr;
        }
}

/*
* Called when the avatar touches the door's collision box.
* This function is called automatically by the Map class.
* It will cause the door to open if it's closed.
*/
void Door::handle_touch()
{
        if ( _state == DS_closed )
        {
                _state = DS_opening;
                _move_ival = new CLerpNodePathInterval( "Door.moveUp", DOOR_SPEED_DIVISOR / _speed,
                                                        CLerpInterval::BT_ease_in_out, true, false, _door_np, NodePath() );
                _move_ival->set_start_pos( LVecBase3( 0.0, 0.0, 0.0 ) );
                _move_ival->set_end_pos( _move_dir );
                _move_ival->start();
        }
}

/////////////////////// BAM STUFF //////////////////////////

void Door::register_with_read_factory()
{
        BamReader::get_factory()->register_factory( get_class_type(), make_from_bam );
}

void Door::write_datagram( BamWriter *manager, Datagram &dg )
{
        PandaNode::write_datagram( manager, dg );

        _move_dir.write_datagram( dg );
        dg.add_int8( _spawn_pos );
        dg.add_stdfloat( _wait );
        dg.add_stdfloat( _speed );
        dg.add_string( _locked_sentence );
        dg.add_string( _unlocked_sentence );
}

TypedWritable *Door::make_from_bam( const FactoryParams &params )
{
        Door *node = new Door( "" );
        DatagramIterator scan;
        BamReader *manager;

        parse_params( params, scan, manager );
        node->fillin( scan, manager );

        return node;
}

void Door::fillin( DatagramIterator &scan, BamReader *manager )
{
        PandaNode::fillin( scan, manager );

        _move_dir.read_datagram( scan );
        _spawn_pos = (SpawnPos)scan.get_int8();
        _wait = scan.get_stdfloat();
        _speed = scan.get_stdfloat();
        _locked_sentence = scan.get_string();
        _unlocked_sentence = scan.get_string();

        if ( _spawn_pos == SP_closed )
        {
                _state = DS_closed;
        }
        else if ( _spawn_pos == SP_opened )
        {
                _state = DS_opened;
        }
}