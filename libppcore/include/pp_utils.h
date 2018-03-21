#ifndef PPUTILS_H
#define PPUTILS_H

#include "config_pandaplus.h"

#include <genericAsyncTask.h>
#include <cLerpNodePathInterval.h>

class EXPCL_PANDAPLUS SerialNumGen
{
public:
        SerialNumGen( int start = 0 );

        int next();

private:
        int _counter;
};

class EXPCL_PANDAPLUS FrameDelayedCall
{
public:

        typedef void CallbackFunc( void * );
        typedef bool CancelFunc( void * );

        FrameDelayedCall( const string &name, CallbackFunc *callback, void *data, int frames = 1, CancelFunc *cancelfunc = NULL );
        FrameDelayedCall();

        void destroy();
        void finish();

        bool is_finished() const;

private:
        CallbackFunc *_callback;
        CancelFunc *_cancel_func;
        void *_data;
        string _name;
        int _num_frames;
        bool _finished;
        string _task_name;
        int _counter;

        PT( GenericAsyncTask ) _task;

        void start_task();
        void stop_task();
        static AsyncTask::DoneStatus tick_task( GenericAsyncTask *task, void *data );
};

class EXPCL_PANDAPLUS PPUtils
{
public:
        static void mount_multifile( const string &mfpath );
        static PT( CLerpNodePathInterval ) make_nodepath_interval( const string &name, double duration, const NodePath &node );

        static SerialNumGen serial_gen;
        static int serial_num();
        static string unique_name( const string &name );
};

#define TypeDecl(classname, parentname)\
private:\
    static TypeHandle _type_handle;\
public:\
  static TypeHandle get_class_type() {\
    return _type_handle;\
  }\
  static void init_type() {\
    parentname::init_type();\
    register_type(_type_handle, #classname,\
                  parentname::get_class_type());\
  }\
  virtual TypeHandle get_type() const {\
    return classname::get_class_type();\
  }\
  virtual TypeHandle force_init_type() { init_type(); return get_class_type(); }

#define TypeDef(classname)\
TypeHandle classname::_type_handle;

#endif // PPUTILS_H