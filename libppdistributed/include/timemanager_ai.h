#ifndef TIMEMANAGERAI_H
#define TIMEMANAGERAI_H

#include "distributedobject_ai.h"

/**
* Server/AI-side implementation of the TimeManager distributed object.
*/
class EXPCL_PPDISTRIBUTED TimeManagerAI : public DistributedObjectAI
{
        DClassDecl( TimeManagerAI, DistributedObjectAI );

public:

        void request_server_time( uint8_t context );
        DCFuncDecl( request_server_time_field );

        virtual void announce_generate();

        InitTypeStart();

        DCFieldDefStart();

        DCFieldDef( server_time, nullptr, nullptr );
        DCFieldDef( request_server_time, request_server_time_field, nullptr );

        DCFieldDefEnd();

        InitTypeEnd();
};

#endif // TIMEMANAGERAI_H