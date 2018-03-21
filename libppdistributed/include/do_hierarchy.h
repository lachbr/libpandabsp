#ifndef DOHIERARCHY_H
#define DOHIERARCHY_H

#include <pmap.h>
#include <pvector.h>

#include "config_pandaplus.h"
#include "pp_dcbase.h"

class DistributedObjectBase;

class EXPCL_PPDISTRIBUTED DoHierarchy
{
public:
        DoHierarchy();

        bool is_empty() const;

        size_t get_length() const;

        void clear();

        void store_object_location( DistributedObjectBase *dobj, DOID_TYPE parentid, ZONEID_TYPE zoneid );
        void delete_object_location( DistributedObjectBase *dobj, DOID_TYPE parentid, ZONEID_TYPE zoneid );

private:
        typedef vector<DOID_TYPE> DoIdVec;
        typedef pmap<ZONEID_TYPE, DoIdVec> Zone2ChildId;
        typedef pmap<DOID_TYPE, Zone2ChildId> ParentMapThing;

        ParentMapThing _table;
        DoIdVec _all_do_ids;
};

#endif // DOHEIRARCHY_H