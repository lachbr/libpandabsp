#include "distributedRootAI.h"

DClassDef( DistributedRootAI );

void DistributedRootAI::SetParentingRules( string, string )
{
}

void DistributedRootAI::SetParentingRulesField( DCPacker &packer, void *data )
{
        ( ( DistributedRootAI * )data )->SetParentingRules( packer.unpack_string(), packer.unpack_string() );
}