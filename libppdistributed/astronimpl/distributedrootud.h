#ifndef DISTROOTUD_H
#define DISTROOTUD_H

#include "distributedRootAI.h"

class EXPCL_PPDISTRIBUTED DistributedRootUD : public DistributedRootAI
{
        DClassDecl( DistributedRootUD, DistributedRootAI );

        InitTypeStart();
        DCFieldDefStart();

        DCFieldDef( SetParentingRules, SetParentingRulesField, NULL );

        DCFieldDefEnd();
        InitTypeEnd();
};

#endif // DISTROOTUD_H