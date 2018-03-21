#ifndef CLIENT_REPOSITORY_H
#define CLIENT_REPOSITORY_H

class UniqueIdAllocator;

#include "clientrepositorybase.h"

NotifyCategoryDeclNoExport( clientRepository );

class EXPCL_PPDISTRIBUTED ClientRepository : public ClientRepositoryBase
{
public:
        ClientRepository();
        ClientRepository( vector<string> &dc_files, const string &dc_suffix, ConnectMethod cm = CM_HTTP );

        virtual ~ClientRepository();

        void handle_set_doid_range( DatagramIterator &di );

        void create_ready();

        void handle_request_generates( DatagramIterator &di );

        void resend_generate( DistributedObjectBase *obj );

        void handle_generate( DatagramIterator &di );

        DOID_TYPE allocate_do_id();

        DOID_TYPE reserve_do_id( DOID_TYPE do_id );

        void free_do_id( DOID_TYPE do_id );

        void store_object_location( DistributedObjectBase *object, DOID_TYPE parent_id, ZONEID_TYPE zone_id );

        virtual DistributedObjectBase *create_distributed_object( DistributedObjectBase *object, ZONEID_TYPE zone_id = 0,
                                                                  bool explicit_doid = false, DOID_TYPE do_id = 0,
                                                                  DCClassPP *dclass = NULL );

        //void FormatGenerate()

        void send_delete_msg( DOID_TYPE do_id );

        void send_disconnect();

        void set_interest_zones( vector<ZONEID_TYPE> &interest_zone_ids );

        void set_object_zone( DistributedObject *obj, ZONEID_TYPE zone_id );

        virtual void send_set_location( DOID_TYPE do_id, DOID_TYPE parent_id, ZONEID_TYPE zone_id );

        void send_heartbeat();

        bool is_local_id( DOID_TYPE do_id ) const;

        bool have_create_authority() const;

        virtual DOID_TYPE get_avatar_id_from_sender() const;

        void handle_datagram( DatagramIterator &di );

        void handle_message_type( uint16_t msg_type, DatagramIterator &di );

        void handle_update_field( DatagramIterator &di );

        void handle_disable( DatagramIterator &di );

        void handle_delete( DatagramIterator &di );

        void delete_object( DOID_TYPE do_id );

        CHANNEL_TYPE get_our_channel() const;

private:
        UniqueIdAllocator *_do_id_allocator;
        DOID_TYPE _do_id_base;
        DOID_TYPE _do_id_last;

        CHANNEL_TYPE _our_channel;

        CHANNEL_TYPE _current_sender_id;

        vector<ZONEID_TYPE> _interest_zones;
};

#endif // CLIENT_REPOSITORY_H