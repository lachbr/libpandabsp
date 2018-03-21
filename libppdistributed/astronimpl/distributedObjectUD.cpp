#include "distributedObjectUD.h"
#include <throw_event.h>
#include "pp_globals.h"

#include "aiRepositoryBase.h"

DClassDef( DistributedObjectUD );

NotifyCategoryDef( distributedObjectUD, "" );

DistributedObjectUD::DistributedObjectUD()
        : DistributedObjectAI()
{
}

void DistributedObjectUD::SetLocation( DOID_TYPE parent, ZONEID_TYPE zone )
{
        gpAIR->StoreObjectLocation( this, parent, zone );
}