#include "distributedObjectGlobal.h"

DClassDef_omitND( DistributedObjectGlobal );
bool DistributedObjectGlobal::s_bNeverDisable = true;

DistributedObjectGlobal::DistributedObjectGlobal()
        : DistributedObject()
{
        m_parentId = 0;
        m_zoneId = 0;
}