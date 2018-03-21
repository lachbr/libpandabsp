#ifndef DISTROOT_H
#define DISTROOT_H

#include "distributedObject.h"

/**
* Simple class used as a "root" object for parenting.
*/
class EXPCL_PPDISTRIBUTED DistributedRoot : public DistributedObject
{
        DClassDecl( DistributedRoot, DistributedObject );

        InitTypeStart();
        DCFieldDefStart();

        DCFieldDef( SetParentingRules, NULL, NULL );

        DCFieldDefEnd();
        InitTypeEnd();
};

#endif // DISTROOT_H