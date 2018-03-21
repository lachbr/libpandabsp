#ifndef DISTROOTAI_H
#define DISTROOTAI_H

#include "distributedObjectAI.h"

class EXPCL_PPDISTRIBUTED DistributedRootAI : public DistributedObjectAI
{
        DClassDecl( DistributedRootAI, DistributedObjectAI );

public:
        void SetParentingRules( string, string );
        static void SetParentingRulesField( DCPacker &packer, void *data );

        InitTypeStart();
        DCFieldDefStart();

        DCFieldDef( SetParentingRules, SetParentingRulesField, NULL );

        DCFieldDefEnd();
        InitTypeEnd();
};

#endif // DISTROOTAI_H