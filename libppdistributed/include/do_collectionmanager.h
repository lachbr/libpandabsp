#ifndef DOCOLLECTIONMANAGER_H
#define DOCOLLECTIONMANAGER_H

#include <pmap.h>

#include "config_ppdistributed.h"
#include "do_hierarchy.h"

class DatagramIterator;

class EXPCL_PPDISTRIBUTED DoCollectionManager
{
public:
        DoCollectionManager();

        typedef pmap<DOID_TYPE, DistributedObjectBase *> DoMap;

        DistributedObjectBase *get_do( DOID_TYPE doid );
        DistributedObjectBase *get_owner_view( DOID_TYPE doid );
        int get_game_do_id() const;

        DoMap &get_do_table( bool ownerview );

        DistributedObjectBase *do_find( const string &str );
        vector<DistributedObjectBase *> do_find_all( const string &str );


        bool has_owner_view_do_id( DOID_TYPE doid ) const;

        vector<DistributedObjectBase *> get_all_of_type( TypeHandle &handle );
	vector<DistributedObjectBase *> get_all_exact_type( TypeHandle &handle );

        void handle_object_location( DatagramIterator &di );

        void store_object_location( DistributedObjectBase *dobj, DOID_TYPE parentid, ZONEID_TYPE zone );

        void delete_object_location( DistributedObjectBase *dobj, DOID_TYPE parentid, ZONEID_TYPE zone );

        void add_do_to_tables( DistributedObjectBase *dobj, DOID_TYPE parent, ZONEID_TYPE zone, bool ownerview = false );
        void remove_do_from_tables( DistributedObjectBase *dobj );

        DoMap get_objects_in_zone( DOID_TYPE parent, ZONEID_TYPE zone );
        DoMap get_objects_of_class_in_zone( DOID_TYPE parent, ZONEID_TYPE zone, TypeHandle &handle );

protected:
        DoMap _do_id_2_do;
        DoMap _do_id_2_owner_view;

        DoHierarchy _do_hierarchy;
};

#endif // DOCOLLECTIONMANAGER_H