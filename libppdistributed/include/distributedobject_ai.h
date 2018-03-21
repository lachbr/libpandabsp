#ifndef DISTRIBUTED_OBJECT_AI_H
#define DISTRIBUTED_OBJECT_AI_H

#include "distributedobject_base.h"

NotifyCategoryDeclNoExport( distributedObjectAI );

class EXPCL_PPDISTRIBUTED DistributedObjectAI : public DistributedObjectBase
{
        DClassDecl( DistributedObjectAI, DistributedObjectBase );

public:
        DistributedObjectAI();

        static ZONEID_TYPE _quiet_zone;

        string get_delete_event() const;

        void send_delete_event();

        bool get_cacheable() const;
        void delete_or_delay();

        int get_delay_delete_count() const;

        void delete_do();

        bool is_deleted() const;

        bool is_generated() const;

        virtual void announce_generate();
        void add_interest( ZONEID_TYPE zone_id, string note = "", string event = "" );

        virtual void set_location( DOID_TYPE parent_id, ZONEID_TYPE zone_id );
        void d_set_location( DOID_TYPE parent_id, ZONEID_TYPE zone_id );
        void b_set_location( DOID_TYPE parent_id, ZONEID_TYPE zone_id );

        DCFuncDecl( get_location_field );

        void post_generate_message();

        virtual void update_required_fields( DCClassPP *dclass, DatagramIterator &di );
        virtual void update_all_required_fields( DCClassPP *dclass, DatagramIterator &di );
        virtual void update_required_other_fields( DCClassPP *dclass, DatagramIterator &di );
        virtual void update_all_required_other_fields( DCClassPP *dclass, DatagramIterator &di );

        string get_zone_change_event() const;
        static string get_zone_change_event_static( DOID_TYPE do_id );

        string get_logical_zone_change_event() const;
        static string get_logical_zone_change_event_static( DOID_TYPE do_id );

        void handle_logical_zone_change( ZONEID_TYPE new_zoneid, ZONEID_TYPE old_zoneid );

        CHANNEL_TYPE get_puppet_connection_channel( DOID_TYPE do_id );
        CHANNEL_TYPE get_account_connection_channel( DOID_TYPE do_id );
        DOID_TYPE get_account_id_from_channel_code( CHANNEL_TYPE channel );
        DOID_TYPE get_avatar_id_from_channel_code( CHANNEL_TYPE channel );

#ifdef ASTRON_IMPL

        void pre_allocate_do_id();

        void generate_with_required( ZONEID_TYPE zone_id );
        void generate_with_required_and_id( DOID_TYPE do_id, DOID_TYPE parent_id, ZONEID_TYPE zone_id );

        virtual void generate();
        virtual void generate_init();

        void send_generate_with_required( DOID_TYPE parent_id, ZONEID_TYPE zone_id );
        void init_from_server_response();

        void request_delete();

        bool validate( DOID_TYPE do_id, bool flag, const string &msg );

        CHANNEL_TYPE generate_target_channel() const;

#endif // ASTRON_IMPL

        InitTypeStart();
        DCFieldDefStart();

        DCFieldDef( set_location, set_location_field, get_location_field );

        DCFieldDefEnd();
        InitTypeEnd();

protected:
        string _account_name;
        bool _requested_delete;
        ZONEID_TYPE _last_non_quiet_zone;

private:
        int _next_barrier_context;
        bool _generated;
        int _generates;
        bool _prealloc_do_id;
};

#endif // DISTRIBUTED_OBJECT_AI_H