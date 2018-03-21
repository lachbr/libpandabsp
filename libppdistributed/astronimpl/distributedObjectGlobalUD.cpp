#include "distributedObjectGlobalUD.h"

#include "pp_globals.h"

#include "aiRepositoryBase.h"

DClassDef_omitGD( DistributedObjectGlobalUD );
bool DistributedObjectGlobalUD::s_bGlobalDo = true;

void DistributedObjectGlobalUD::AnnounceGenerate()
{
        ( ( AIRepositoryBase * )gpAIR )->RegisterForChannel( m_doId );
        DistributedObjectUD::AnnounceGenerate();
}

void DistributedObjectGlobalUD::DeleteDo()
{
        ( ( AIRepositoryBase * )gpAIR )->UnregisterForChannel( m_doId );
        DistributedObjectUD::DeleteDo();
}