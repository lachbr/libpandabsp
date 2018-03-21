#include "timemanager_ai.h"
#include "pp_globals.h"
#include "clientrepository.h"
#include "clockdelta.h"

DClassDef( TimeManagerAI );

void TimeManagerAI::
announce_generate()
{
        cout << "TimeManagerAI: announceGenerate" << endl;
        DistributedObjectAI::announce_generate();
}

void TimeManagerAI::
request_server_time( uint8_t context )
{
        int32_t timestamp = ClockDelta::get_global_ptr()->get_real_network_time( 32 );

        DOID_TYPE requester = g_air->get_avatar_id_from_sender();

#ifdef ASTRON_IMPL
        BeginAIUpdate_CH( "server_time", requester );
#else // ASTRON_IMPL
        BeginAIUpdate( "server_time" );
#endif // ASTRON_IMPL

        AddArg( uint, context );
        AddArg( int, timestamp );

#ifdef ASTRON_IMPL
        SendAIUpdate( "server_time" );
#else // ASTRON_IMPL
        SendAIUpdate_CH( "server_time", requester );
#endif // ASTRON_IMPL
}

void TimeManagerAI::
request_server_time_field( DCPacker &packer, void *data )
{
        uint8_t context = packer.unpack_uint();
        ( ( TimeManagerAI * )data )->request_server_time( context );
}