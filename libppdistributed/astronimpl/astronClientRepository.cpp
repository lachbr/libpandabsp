#include "astronClientRepository.h"
#include <throw_event.h>

#include "distributedObjectBase.h"

AstronClientRepository::AstronClientRepository( ConnectionRepository::ConnectMethod cm, vector<string> &dc_files )
        : ClientRepositoryBase( "", cm, dc_files )
{
}

void AstronClientRepository::HandleDatagram( DatagramIterator &di )
{
        unsigned int msgtype = get_msg_type();
        switch ( msgtype )
        {
        case CLIENT_HELLO_RESP:
                HandleHelloResp( di );
                break;
        case CLIENT_EJECT:
                HandleEject( di );
                break;
        case CLIENT_ENTER_OBJECT_REQUIRED:
                HandleEnterObjectRequired( di );
                break;
        case CLIENT_ENTER_OBJECT_REQUIRED_OTHER:
                HandleEnterObjectRequiredOther( di );
                break;
        case CLIENT_ENTER_OBJECT_REQUIRED_OTHER_OWNER:
                HandleEnterObjectRequiredOtherOwner( di );
                break;
        case CLIENT_ENTER_OBJECT_REQUIRED_OWNER:
                HandleEnterObjectRequiredOwner( di );
                break;
        case CLIENT_OBJECT_SET_FIELD:
                HandleUpdateField( di );
                break;
        case CLIENT_OBJECT_LEAVING:
        case CLIENT_OBJECT_LEAVING_OWNER:
                HandleObjectLeaving( di );
                break;
        case CLIENT_OBJECT_LOCATION:
                HandleObjectLocation( di );
                break;
        case CLIENT_ADD_INTEREST:
                HandleAddInterest( di );
                break;
        case CLIENT_ADD_INTEREST_MULTIPLE:
                HandleAddInterestMultiple( di );
                break;
        case CLIENT_REMOVE_INTEREST:
                HandleRemoveInterest( di );
                break;
        case CLIENT_DONE_INTEREST_RESP:
                HandleInterestDoneMessage( di );
                break;
        }
}

void AstronClientRepository::HandleHelloResp( DatagramIterator &di )
{
        throw_event( "CLIENT_HELLO_RESP" );
}

void AstronClientRepository::HandleEject( DatagramIterator &di )
{
        uint16_t ec = di.get_uint16();
        string reason = di.get_string();
        throw_event( "CLIENT_EJECT", EventParameter( ec ), EventParameter( reason ) );
}

void AstronClientRepository::HandleEnterObjectRequired( DatagramIterator &di )
{
        DOID_TYPE doid = di.get_uint32();
        DOID_TYPE parent = di.get_uint32();
        ZONEID_TYPE zone = di.get_uint32();
        uint16_t dclassid = di.get_uint16();
        DCClassPP *dclass = m_mDClassByNumber[dclassid];
        nassertv( dclass != NULL );
        GenerateWithRequiredFields( dclass, doid, di, parent, zone );
}

void AstronClientRepository::HandleEnterObjectRequiredOther( DatagramIterator &di )
{
        DOID_TYPE doid = di.get_uint32();
        DOID_TYPE parent = di.get_uint32();
        ZONEID_TYPE zone = di.get_uint32();
        uint16_t dclassid = di.get_uint16();
        DCClassPP *dclass = m_mDClassByNumber[dclassid];
        nassertv( dclass != NULL );
        GenerateWithRequiredFields( dclass, doid, di, parent, zone );
}

void AstronClientRepository::HandleEnterObjectRequiredOtherOwner( DatagramIterator &di )
{
        DOID_TYPE doid = di.get_uint32();
        DOID_TYPE parent = di.get_uint32();
        ZONEID_TYPE zone = di.get_uint32();
        uint16_t dclassid = di.get_uint16();
        DCClassPP *dclass = m_mDClassByNumber[dclassid];
        nassertv( dclass != NULL );
        GenerateWithRequiredFieldsOwner( dclass, doid, di );
}

void AstronClientRepository::HandleEnterObjectRequiredOwner( DatagramIterator &di )
{
        DOID_TYPE doid = di.get_uint32();
        DOID_TYPE parent = di.get_uint32();
        ZONEID_TYPE zone = di.get_uint32();
        uint16_t dclassid = di.get_uint16();
        DCClassPP *dclass = m_mDClassByNumber[dclassid];
        nassertv( dclass != NULL );
        GenerateWithRequiredFieldsOwner( dclass, doid, di );
}

DistributedObjectBase *AstronClientRepository::GenerateWithRequiredFieldsOwner( DCClassPP *dclass, DOID_TYPE doid, DatagramIterator &di )
{
        DistributedObjectBase *obj;
        if ( HasOwnerViewDoId( doid ) )
        {
                obj = m_mDoId2OwnerView[doid];
                nassertr( obj->GetDClass() == dclass, NULL );
                obj->Generate();
                obj->UpdateRequiredFields( dclass, di );

                // CACHE OWNER STUFF!!!! else if!!!

        }
        else
        {
                obj = dclass->GetObjSingleton()->MakeNew();
                obj->SetDClass( dclass );
                obj->SetDoId( doid );
                m_mDoId2OwnerView[doid] = obj;
                obj->GenerateInit();
                obj->Generate();
                obj->UpdateRequiredFields( dclass, di );
        }
        return obj;
}

void AstronClientRepository::HandleObjectLeaving( DatagramIterator &di )
{
        DOID_TYPE doid = di.get_uint32();
        DeleteObject( doid );
        throw_event( "CLIENT_OBJECT_LEAVING", EventParameter( ( double )doid ) );
}

void AstronClientRepository::HandleAddInterest( DatagramIterator &di )
{
        uint32_t context = di.get_uint32();
        uint16_t interestid = di.get_uint16();
        DOID_TYPE parentid = di.get_uint32();
        ZONEID_TYPE zone = di.get_uint32();
        throw_event( "CLIENT_ADD_INTEREST", EventParameter( ( double )context ),
                     EventParameter( interestid ), EventParameter( ( double )parentid ),
                     EventParameter( ( double )zone ) );
}

void AstronClientRepository::HandleAddInterestMultiple( DatagramIterator &di )
{
        uint32_t context = di.get_uint32();
        uint16_t interestid = di.get_uint16();
        DOID_TYPE parentid = di.get_uint32();
        vector<ZONEID_TYPE> zones;
        uint16_t num_zones = di.get_uint16();
        for ( uint16_t i = 0; i < num_zones; i++ )
        {
                zones.push_back( di.get_uint32() );
        }
        throw_event( "CLIENT_ADD_INTEREST_MULTIPLE", EventParameter( ( double )context ),
                     EventParameter( interestid ), EventParameter( ( double )parentid ) );
}

void AstronClientRepository::HandleRemoveInterest( DatagramIterator &di )
{
        uint32_t context = di.get_uint32();
        uint16_t interest = di.get_uint16();
        throw_event( "CLIENT_REMOVE_INTEREST", EventParameter( ( double )context ),
                     EventParameter( interest ) );
}

void AstronClientRepository::DeleteObject( DOID_TYPE doid )
{
        if ( GetDo( doid ) != NULL )
        {
                DistributedObjectBase *obj = m_mDoId2Do[doid];
                m_mDoId2Do.erase( m_mDoId2Do.find( doid ) );
                obj->DeleteOrDelay();
                if ( IsLocalId( doid ) )
                {
                        //free_doid(doid);
                }
        }

        // else if cache stuff

        else
        {
                //ignore it
        }
}

void AstronClientRepository::SendHello( const string &ver_str )
{
        Datagram dg;
        dg.add_uint16( CLIENT_HELLO );
        dg.add_uint32( m_ulHashVal );
        dg.add_string( ver_str );
        Send( dg );
}

void AstronClientRepository::SendHeartbeat()
{
        Datagram dg;
        dg.add_uint16( CLIENT_HEARTBEAT );
        Send( dg );
}

void AstronClientRepository::SendAddInterest( int context, int interestid, DOID_TYPE parent, ZONEID_TYPE zone )
{
        Datagram dg;
        dg.add_uint16( CLIENT_ADD_INTEREST );
        dg.add_uint32( context );
        dg.add_uint16( interestid );
        dg.add_uint32( parent );
        dg.add_uint32( zone );
        Send( dg );
}

void AstronClientRepository::SendAddInterestMultiple( int context, int interestid, DOID_TYPE parent, vector<ZONEID_TYPE> zones )
{
        Datagram dg;
        dg.add_uint16( CLIENT_ADD_INTEREST_MULTIPLE );
        dg.add_uint32( context );
        dg.add_uint16( interestid );
        dg.add_uint32( parent );
        dg.add_uint16( zones.size() );
        for ( size_t i = 0; i < zones.size(); i++ )
        {
                dg.add_uint32( zones[i] );
        }
        Send( dg );
}

void AstronClientRepository::SendRemoveInterest( int context, int interestid )
{
        Datagram dg;
        dg.add_uint16( CLIENT_REMOVE_INTEREST );
        dg.add_uint32( context );
        dg.add_uint16( interestid );
        Send( dg );
}

void AstronClientRepository::LostConnection()
{
        throw_event( "LOST_CONNECTION" );
}

void AstronClientRepository::disconnect()
{
        for ( DoMap::iterator itr = m_mDoId2Do.begin(); itr != m_mDoId2Do.end(); ++itr )
        {
                DeleteObject( itr->first );
        }
        ClientRepositoryBase::disconnect();
}
