#ifndef CLIENTREPOSITORYBASE_H
#define CLIENTREPOSITORYBASE_H

#include "dcclass_pp.h"
#include "parentmgr.h"
#include "relatedobjectmgr.h"
#include "connectionrepository.h"

#include <genericAsyncTask.h>

// Forward declarations
class DistributedObjectBase;
class TimeManager;

NotifyCategoryDeclNoExport( clientRepositoryBase );

class EXPCL_PPDISTRIBUTED ClientRepositoryBase : public ConnectionRepository
{
public:
        ClientRepositoryBase();
        ClientRepositoryBase( const string &dc_suffix, ConnectMethod cm, vector<string> &dc_files = vector<string>() );

        int allocate_context();

        void do_generate( uint32_t parent_id, uint32_t zone_id, uint16_t class_id, uint32_t do_id, DatagramIterator &di );
        DistributedObjectBase *generate_with_required_fields( DCClassPP *dclass, uint32_t do_id,
                                                              DatagramIterator &di,
                                                              uint32_t parent_id = 0,
                                                              uint32_t zone_id = 0 );
        DistributedObjectBase *generate_with_required_other_fields( DCClassPP *dclass, uint32_t do_id,
                                                                    DatagramIterator &di,
                                                                    uint32_t parent_id = 0,
                                                                    uint32_t zone_id = 0 );
        void disable_do_id( uint32_t do_id, bool owner_view = false );

        void set_server_delta( PN_stdfloat delta );
        PN_stdfloat get_server_delta() const;

        PN_stdfloat get_server_time_of_day() const;

        typedef pmap<uint32_t, DistributedObjectBase *> ObjectsDict;

        ObjectsDict get_objects_of_class( DistributedObjectBase *cls );
        ObjectsDict get_objects_of_exact_class( DistributedObjectBase *cls );

        void consider_heartbeat();
        void stop_heartbeat();
        void start_heartbeat();
        static AsyncTask::DoneStatus send_heartbeat_task( GenericAsyncTask *task, void *data );
        virtual void send_heartbeat() = 0;
        void wait_for_next_heartbeat();

        NodePath get_world( uint32_t do_id );

        bool is_local_id( uint32_t do_id );

        void set_time_manager( TimeManager *tm );
        TimeManager *get_time_manager() const;

        ParentMgr &get_parent_mgr();

        void set_local_avatar_do_id( DOID_TYPE do_id );
        DOID_TYPE get_local_avatar_do_id() const;

protected:
        virtual void handle_delete( DatagramIterator &di );

        void handle_update_field( DatagramIterator &di );
        void handle_go_get_lost( DatagramIterator &di );
        void handle_server_heartbeat( DatagramIterator &di );
        string handle_system_message( DatagramIterator &di );
        void handle_system_message_acknowledge( DatagramIterator &di );

        RelatedObjectMgr _related_object_mgr;
        ParentMgr _parent_mgr;

        TimeManager *_time_mgr;

        DOID_TYPE _local_avatar_do_id;

private:
        double _server_delta;
        double _heartbeat_interval;
        int _context;
        double _last_generate;
        double _last_heartbeat;
        int _special_name_num;
        bool _heartbeat_started;
        bool _no_defer;

        PT( GenericAsyncTask ) _heartbeat_task;

        void do_update( uint32_t do_id, DatagramIterator &di, bool ov_updated );
};

#endif // CLIENTREPOSITORYBASE_H