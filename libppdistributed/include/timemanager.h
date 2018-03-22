#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <notifyCategoryProxy.h>
#include <genericAsyncTask.h>
#include <event.h>

#include "distributedobject.h"

NotifyCategoryDeclNoExport( timeManager );

/**
 * Client-side implementation of the TimeManager distributed object.
 */
class EXPCL_PPDISTRIBUTED TimeManager : public DistributedObject
{
        DClassDecl( TimeManager, DistributedObject );

public:

        TimeManager();

        void server_time( uint8_t context, int32_t timestamp );
        DCFuncDecl( server_time_field );

        void d_request_server_time( uint8_t context );

        virtual void generate();
        virtual void announce_generate();
        virtual void disable();
        virtual void delete_do();

        void start_task();
        void stop_task();

        bool synchronize( const string &desc );

        InitTypeStart();

        DCFieldDefStart();

        DCFieldDef( server_time, server_time_field, nullptr );
        DCFieldDef( request_server_time, nullptr, nullptr );

        DCFieldDefEnd();

        InitTypeEnd();

private:
        uint8_t _this_context;
        uint8_t _next_context;
        int _attempt_count;
        double _start;
        double _last_attempt;
        int _task_result;

        PT( GenericAsyncTask ) _update_task;

        static AsyncTask::DoneStatus do_update_task( GenericAsyncTask *task, void *data );

        static void handle_clock_error_event( const Event *e, void *data );
};

#endif // TIMEMANAGER_H