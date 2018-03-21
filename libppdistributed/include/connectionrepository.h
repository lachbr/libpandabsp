#ifndef CONNECTION_REPOSITORY_H
#define CONNECTION_REPOSITORY_H

#include <cConnectionRepository.h>
#include <httpClient.h>
#include <urlSpec.h>
#include <datagramIterator.h>
#include <datagram.h>
#include <genericAsyncTask.h>
#include <notifyCategoryProxy.h>
#include <pmap.h>

#include "do_interestmanager.h"
#include "pp_dcbase.h"
#include "config_ppdistributed.h"

NotifyCategoryDeclNoExport( connectionRepository );

class EXPCL_PPDISTRIBUTED ConnectionRepository : public CConnectionRepository, public DoInterestManager
{
public:
        enum ConnectMethod
        {
                CM_HTTP,
                CM_NET,
                CM_NATIVE
        };

        typedef void ConnectCallbackFunc( void * );
        typedef void ConnectFailCallbackFunc( void *, int, const string &, const string &, int );
        typedef vector<URLSpec> vector_url;

        class HTTPConnectProcess
        {
        public:
                HTTPConnectProcess( ConnectionRepository *repo, PT( HTTPChannel ) ch, vector_url &servers,
                                    ConnectCallbackFunc *callback, ConnectFailCallbackFunc *failcallback, void *data, int start_index = 0 );
                ~HTTPConnectProcess();
        private:
                void connect_callback();
                AsyncTask::DoneStatus poll_connection();
                static AsyncTask::DoneStatus poll_connection_task( GenericAsyncTask *task, void *data );
        private:
                void *_data;
                int _index;
                ConnectionRepository *_repo;
                ConnectCallbackFunc *_callback;
                ConnectFailCallbackFunc *_fail_callback;
                vector_url _servers;
                PT( GenericAsyncTask ) _poll_task;
                PT( HTTPChannel ) _ch;
        };

        static string get_garbage_collect_task_name();
        static string get_garbage_threshold_task_name();

        ConnectionRepository( vector<string> &dc_files, ConnectMethod cm, bool owner_view = false );
        ConnectionRepository();

        void connect( vector<URLSpec> &servers, ConnectCallbackFunc *callback, ConnectFailCallbackFunc *failcallback, void *data );
        DistributedObjectBase *generate_global_object( uint32_t do_id, const string &dc_name );
        void read_dc_file();
        virtual void handle_datagram( DatagramIterator &di );
        PT( HTTPClient ) check_http();
        void send( Datagram &dg );

        void disconnect();
        void shutdown();

        void send_add_interest( int handle, int contextid, DOID_TYPE parentid,
                                vector<ZONEID_TYPE> zoneidlist, const string &desc = "" );
        void send_remove_interest( int handle, int contextid );
        void send_remove_ai_interest( int handle );

        //virtual void SendSetLocation( DistributedObjectBase *pObj, DOID_TYPE idParent, ZONEID_TYPE idZone ) = 0;
        virtual void request_delete( DistributedObjectBase *obj );

        virtual DOID_TYPE get_avatar_id_from_sender() const = 0;

        DCClassPP *get_dclass_by_name( const string &name ) const;

protected:
        string _dc_suffix;
        string _booted_text;
        int _booted_index;

        void start_reader_poll_task();
        void stop_reader_poll_task();

        void lost_connection();

        virtual void handle_update_field_owner();
        virtual void handle_update_field();

private:
        void handle_connected_http( HTTPChannel *ch, URLSpec &url );

        bool reader_poll_once();
        void reader_poll_until_empty();
        static AsyncTask::DoneStatus reader_poll_task( GenericAsyncTask *task, void *data );



protected:
        unsigned long _hash_val;

        DCFieldPP *get_field_wrapper( DCField *field );

        DatagramIterator _private_di;
        URLSpec _server_address;
        PT( HTTPClient ) _http;

        PT( GenericAsyncTask ) _reader_poll_task;
        ConnectMethod _connect_method;
        vector<string> _dc_files;
        pmap<string, DCClassPP *> _dclass_by_name;
        pmap<int, DCClassPP *> _dclass_by_number;

        vector<DCField *> _wrapped_fields;
        vector<DCFieldPP *> _wrapper_fields;
};

#endif // CONNECTION_REPOSITORY_H