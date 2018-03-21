#ifndef DIST_OBJ_GLOBAL_UD_H
#define DIST_OBJ_GLOBAL_UD_H

#include "distributedObjectUD.h"

class EXPCL_PPDISTRIBUTED DistributedObjectGlobalUD : public DistributedObjectUD
{
        DClassDecl( DistributedObjectGlobalUD, DistributedObjectUD );

public:
        virtual void AnnounceGenerate();
        virtual void DeleteDo();
};

#endif // DIST_OBJ_GLOBAL_UD_H