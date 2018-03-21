#ifndef SERVER_REPOSITORY_H
#define SERVER_REPOSITORY_H

#include <pointerTo.h>
#include <netAddress.h>
#include <queuedConnectionListener.h>
#include <queuedConnectionManager.h>
#include <queuedConnectionReader.h>
#include <connectionWriter.h>
#include <configVariableInt.h>
#include <uniqueIdAllocator.h>
#include <dcFile.h>
#include <genericAsyncTask.h>

#include "pp_dcbase.h"

NotifyCategoryDeclNoExport( serverRepository );

class Connection;

struct Object
{
        DOID_TYPE do_id;
        ZONEID_TYPE zone_id;
        DCClassPP *dclass;
};

typedef vector<Object *> ObjectVec;

typedef map<DOID_TYPE, Object *> ObjectsByDoId;
typedef map<ZONEID_TYPE, ObjectVec> ObjectsByZoneId;

struct Client
{
        PT( Connection ) connection;
        NetAddress net_address;
        DOID_TYPE do_id_base;
        vector<ZONEID_TYPE> explicit_interest_zone_ids;
        vector<ZONEID_TYPE> current_interest_zone_ids;
        ObjectsByDoId objects_by_doid;
        ObjectsByZoneId objects_by_zoneid;

};

typedef vector<string> stringvec_t;
typedef vector<Client *> ClientVec;
typedef map<PT( Connection ), Client *> ClientsByConnection;

class EXPCL_PPDISTRIBUTED ServerRepository
{
protected:
        QueuedConnectionManager _qcm;
        QueuedConnectionListener _qcl;
        QueuedConnectionReader _qcr;
        ConnectionWriter _cw;

        map<string, DCClassPP *> _dclasses_by_name;
        map<int, DCClassPP *> _dclasses_by_number;

        PT( Connection ) _tcp_rendezvous;

        PT( GenericAsyncTask ) _listener_poll_task;
        PT( GenericAsyncTask ) _reader_poll_task;
        PT( GenericAsyncTask ) _client_dc_task;

        ClientsByConnection _clients_by_connection;
        map<DOID_TYPE, Client *> _clients_by_do_id_base;

        map<ZONEID_TYPE, ClientVec> _zones_to_clients;
        map<ZONEID_TYPE, ObjectVec> _objects_by_zone_id;

        ConfigVariableInt _do_id_range;

        UniqueIdAllocator *_id_alloc;

        DCFile _dc_file;
        string _dc_suffix;

        unsigned long _hash_val;

        string _address;
        int _port;
        vector<string> _dc_files;
        vector<DCField *> _wrapped_fields;
        vector<DCFieldPP *> _wrapper_fields;

public:
        ServerRepository( int tcp_port, const string &server_address, stringvec_t &dc_files );
        ~ServerRepository();

        void start_server();

        void set_tcp_header_size( int header_size );
        int get_tcp_header_size() const;

        void read_dc_file( stringvec_t &dc_file_names );

        bool reader_poll_once();

        virtual void handle_datagram( NetDatagram &datagram );

        virtual void handle_message_type( uint16_t msg_type, DatagramIterator &di );

        void handle_client_create_object( NetDatagram &datagram, DatagramIterator &dgi );

        void handle_client_object_update_field( NetDatagram &datagram, DatagramIterator &dgi, bool targeted = false );

        DOID_TYPE get_do_id_base( DOID_TYPE do_id );

        void handle_client_delete_object( NetDatagram &datagram, DOID_TYPE do_id );

        void handle_client_object_set_zone( NetDatagram &datagram, DatagramIterator &dgi );

        void set_object_zone( Client *owner, Object *object, ZONEID_TYPE zone_id );

        void send_do_id_range( Client *client );

        void handle_client_disconnect( Client *client );

        void handle_client_set_interest( Client *client, DatagramIterator &dgi );

        void update_client_interest_zones( Client *client );

        void send_to_zone_except( ZONEID_TYPE zone_id, NetDatagram &datagram, ClientVec &exception_list );

        void send_to_all_except( NetDatagram &datagram, ClientVec &exception_list );

        DCFieldPP *get_field_wrapper( DCField *field );

protected:
        virtual void on_client_disconnect( DOID_TYPE do_id_base );



private:
        //static AsyncTask::DoneStatus FlushTask( GenericAsyncTask *task, void *data );
        static AsyncTask::DoneStatus listener_poll( GenericAsyncTask *task, void *data );
        static AsyncTask::DoneStatus reader_poll( GenericAsyncTask *task, void *data );
        static AsyncTask::DoneStatus client_dc_task( GenericAsyncTask *task, void *data );

};

#endif // SERVER_REPOSITORY_H