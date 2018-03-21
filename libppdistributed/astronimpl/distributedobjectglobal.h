#ifndef DISTOBJGLOBAL_H
#define DISTOBJGLOBAL_H

#include "distributedObject.h"

class EXPCL_PPDISTRIBUTED DistributedObjectGlobal : public DistributedObject
{
        DClassDecl( DistributedObjectGlobal, DistributedObject );

public:
        DistributedObjectGlobal();
};

#endif // DISTOBJGLOBAL_H