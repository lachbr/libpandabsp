#ifndef DISTOBJUD_H
#define DISTOBJUD_H

#include "distributedObjectAI.h"

NotifyCategoryDeclNoExport( distributedObjectUD );

// An uberdog is just a specialized AI, and holds most of
// the same functionality plus a little more.
class EXPCL_PPDISTRIBUTED DistributedObjectUD : public DistributedObjectAI
{
        DClassDecl( DistributedObjectUD, DistributedObjectAI );

public:
        DistributedObjectUD();

        virtual void SetLocation( DOID_TYPE parent_id, ZONEID_TYPE zone_id );

        InitTypeStart();
        DCFieldDefStart();

        DCFieldDef( SetLocation, SetLocationField, GetLocationField );

        DCFieldDefEnd();
        InitTypeEnd();
};

#endif // DISTOBJUD_H