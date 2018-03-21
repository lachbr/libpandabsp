#ifndef DISTOBJGLOBALAI_H
#define DISTOBJGLBOALAI_H

#include "distributedObjectAI.h"

class EXPCL_PPDISTRIBUTED DistributedObjectGlobalAI : public DistributedObjectAI
{
        DClassDecl( DistributedObjectGlobalAI, DistributedObjectAI );

public:
        virtual void AnnounceGenerate();
        virtual void DeleteDo();
};

#endif // DISTOBJGLOBALAI_H