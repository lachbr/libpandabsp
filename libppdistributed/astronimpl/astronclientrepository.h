#ifndef ASTRONCLIENTREPOSITORY_H
#define ASTRONCLIENTREPOSITORY_H

#include "clientRepositoryBase.h"

class EXPCL_PPDISTRIBUTED AstronClientRepository : public ClientRepositoryBase
{
public:

        AstronClientRepository( ConnectMethod cm = CM_HTTP, vector<string> &dc_files = vector<string>() );

        void HandleDatagram( DatagramIterator &di );
        void HandleHelloResp( DatagramIterator &di );
        void HandleEject( DatagramIterator &di );
        void HandleEnterObjectRequired( DatagramIterator &di );
        void HandleEnterObjectRequiredOther( DatagramIterator &di );
        void HandleEnterObjectRequiredOtherOwner( DatagramIterator &di );
        void HandleEnterObjectRequiredOwner( DatagramIterator &di );
        //void handle_update_field(DatagramIterator &di);
        void HandleObjectLeaving( DatagramIterator &di );
        void HandleAddInterest( DatagramIterator &di );
        void HandleAddInterestMultiple( DatagramIterator &di );
        void HandleRemoveInterest( DatagramIterator &di );

        DistributedObjectBase *GenerateWithRequiredFieldsOwner( DCClassPP *dclass, DOID_TYPE doid, DatagramIterator &di );

        void DeleteObject( DOID_TYPE doid );

        void SendHello( const string &ver_str );
        void SendHeartbeat();
        void SendAddInterest( int context, int interest_id, DOID_TYPE parent, ZONEID_TYPE zone );
        void SendAddInterestMultiple( int context, int interest_id, DOID_TYPE parent, vector<ZONEID_TYPE> zones );
        void SendRemoveInterest( int context, int interest_id );

        void LostConnection();

        void disconnect();
};

#endif // ASTRONCLIENTREPOSITORY_H