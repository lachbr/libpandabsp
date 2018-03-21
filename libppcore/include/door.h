#ifndef __DOOR_H__
#define __DOOR_H__

#include <pandaNode.h>
#include <nodePath.h>
#include <cLerpNodePathInterval.h>
#include <genericAsyncTask.h>

#include "config_pandaplus.h"

class EXPCL_PANDAPLUS Door : public PandaNode
{
public:

        enum SpawnPos
        {
                SP_closed,
                SP_opened
        };

        enum DoorState
        {
                DS_closed,
                DS_opening,
                DS_opened,
                DS_closing
        };

        Door( const string &name );

        static void register_with_read_factory();
        virtual void write_datagram( BamWriter *manager, Datagram &dg );

        void set_wait( PN_stdfloat wait );
        PN_stdfloat get_wait() const;

        void set_move_dir( LPoint3f &movedir );
        LPoint3f get_move_dir() const;

        void set_speed( PN_stdfloat speed );
        PN_stdfloat get_speed() const;

        void set_spawn_pos( SpawnPos spawnpos );
        SpawnPos get_spawn_pos() const;

        void set_locked_sentence( const string &locked_sentence );
        string get_locked_sentence() const;

        void set_unlocked_sentence( const string &unlocked_sentence );
        string get_unlocked_sentence() const;

        void set_door_np( NodePath &door_np );
        NodePath get_door_np() const;

        void handle_touch();

        void start_tick();
        void stop_tick();

private:
        PN_stdfloat _wait;
        LPoint3f _move_dir;
        PN_stdfloat _speed;
        SpawnPos _spawn_pos;
        string _locked_sentence;
        string _unlocked_sentence;
        NodePath _door_np;

        PT( GenericAsyncTask ) _tick_task;

        PT( CLerpNodePathInterval ) _move_ival;

        void tick();
        static AsyncTask::DoneStatus tick_task( GenericAsyncTask *task, void *data );

        DoorState _state;
        PN_stdfloat _opened_start_t;

protected:
        static TypedWritable *make_from_bam( const FactoryParams &params );
        void fillin( DatagramIterator &scan, BamReader *manager );

public:
        static TypeHandle get_class_type()
        {
                return _type_handle;
        }
        static void init_type()
        {
                PandaNode::init_type();
                register_type( _type_handle, "Door",
                               PandaNode::get_class_type() );
        }
        virtual TypeHandle get_type() const
        {
                return get_class_type();
        }
        virtual TypeHandle force_init_type()
        {
                init_type();
                return get_class_type();
        }

private:
        static TypeHandle _type_handle;
};

#endif // __DOOR_H__