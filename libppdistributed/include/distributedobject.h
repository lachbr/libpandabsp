#ifndef DISTRIBUTED_OBJECT_H
#define DISTRIBUTED_OBJECT_H

//#include "config_pandaplus.h"
#include "distributedobject_base.h"

NotifyCategoryDeclNoExport( distributedObject );

class EXPCL_PPDISTRIBUTED DistributedObject : public DistributedObjectBase
{
        DClassDecl( DistributedObject, DistributedObjectBase );
public:

        enum ES
        {
                ES_new = 1,
                ES_deleted,
                ES_disabling,
                ES_disabled,
                ES_generating,
                ES_generated
        };

        DistributedObject();

        vector<ZONEID_TYPE> get_auto_interests();
        int _auto_interest_handle;

        void delete_or_delay();

        void disable_announce_and_delete();

        string get_disable_event() const;

        void disable_and_announce();

        virtual void announce_generate();
        virtual void disable();

        bool is_disabled() const;
        bool is_generated() const;

        virtual void delete_do();
        virtual void generate();
        virtual void generate_init();

        void post_generate_message();

        virtual void update_required_fields( DCClassPP *dclass, DatagramIterator &di );
        virtual void update_required_other_fields( DCClassPP *dclass, DatagramIterator &di );
        virtual void update_all_required_fields( DCClassPP *dclass, DatagramIterator &di );

        void send_disable_msg();

        void send_delete_msg();

        void add_interest( ZONEID_TYPE zone, const string &note = "", const string &event = "" );
        void remove_interest( int handle, const string &event = "" );

        void b_set_location( DOID_TYPE parent, ZONEID_TYPE zone );
        void d_set_location( DOID_TYPE parent, ZONEID_TYPE zone );
        virtual void set_location( DOID_TYPE parent, ZONEID_TYPE zone );

        bool is_grid_parent() const;

        bool is_local() const;

        InitTypeStart();

        DCFieldDefStart();

        DCFieldDef( set_location, set_location_field, NULL );

        DCFieldDefEnd();

        InitTypeEnd();

protected:
        ES _active_state;
        int _next_context;

        vector<ZONEID_TYPE> _auto_interests;
        bool _computed_auto_interests;

        void deactivate_do();
        void destroy_do();
};

#endif // DISTRIBUTED_OBJECT_H