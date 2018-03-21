#include "distributedObjectGlobalAI.h"

#include "pp_globals.h"

#include "aiRepositoryBase.h"

DClassDef_omitGD( DistributedObjectGlobalAI );
bool DistributedObjectGlobalAI::s_bGlobalDo = true;

void DistributedObjectGlobalAI::AnnounceGenerate()
{
        DistributedObjectAI::AnnounceGenerate();
        ( ( AIRepositoryBase * )gpAIR )->RegisterForChannel( m_doId );;
}

void DistributedObjectGlobalAI::DeleteDo()
{
        DistributedObjectAI::DeleteDo();
        ( ( AIRepositoryBase * )gpAIR )->UnregisterForChannel( m_doId );
}