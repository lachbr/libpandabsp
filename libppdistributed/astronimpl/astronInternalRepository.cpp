#include "astronInternalRepository.h"
#include "datagram_extensions.h"
#include "pp_globals.h"
#include "distributedObjectAI.h"

#include <throw_event.h>

NotifyCategoryDef( astronInternalRepository, "" );

AstronInternalRepository::AstronInternalRepository( CHANNEL_TYPE base_channel, CHANNEL_TYPE server_id,
                                                    const string &dc_suffix, ConnectionRepository::ConnectMethod cm,
                                                    vector<string> &dc_files )
        : AIRepositoryBase( dc_files, cm )
{
        m_dcSuffix = dc_suffix;
        set_client_datagram( false );
        m_serverId = server_id;
        int max_channels = ConfigVariableInt( "air-channel-allocation", 1000000 );
        m_idAlloc = UniqueIdAllocator( base_channel, base_channel + max_channels );
        m_iContextCounter = 0;
}

AstronDatabaseInterface &AstronInternalRepository::GetDBInterface()
{
        return m_dbInterface;
}

int AstronInternalRepository::GetContext()
{
        m_iContextCounter = ( m_iContextCounter + 1 ) & UINT_LEAST32_MAX;
        return m_iContextCounter;
}

void AstronInternalRepository::RegisterForChannel( CHANNEL_TYPE chan )
{
        if ( find( m_vRegisteredChannels.begin(), m_vRegisteredChannels.end(), chan ) != m_vRegisteredChannels.end() )
        {
                // This channel already exists.
                return;
        }

        m_vRegisteredChannels.push_back( chan );

        Datagram dg;
        DgAddServerControlHeader( dg, CONTROL_ADD_CHANNEL );
        dg.add_uint64( chan );
        Send( dg );
}

void AstronInternalRepository::UnregisterForChannel( CHANNEL_TYPE chan )
{
        pvector<CHANNEL_TYPE>::iterator itr = find( m_vRegisteredChannels.begin(), m_vRegisteredChannels.end(), chan );
        if ( itr == m_vRegisteredChannels.end() )
        {
                return;
        }

        m_vRegisteredChannels.erase( itr );

        Datagram dg;
        DgAddServerControlHeader( dg, CONTROL_REMOVE_CHANNEL );
        dg.add_uint64( chan );
        Send( dg );
}

void AstronInternalRepository::AddPostRemove( Datagram &dg )
{
        Datagram dg2;
        DgAddServerControlHeader( dg2, CONTROL_ADD_POST_REMOVE );
        dg2.add_uint64( m_ourChannel );
        dg2.add_string( dg.get_message() );
        Send( dg2 );
}

void AstronInternalRepository::ClearPostRemove()
{
        Datagram dg;
        DgAddServerControlHeader( dg, CONTROL_CLEAR_POST_REMOVES );
        dg.add_uint64( m_ourChannel );
        Send( dg );
}

void AstronInternalRepository::HandleDatagram( DatagramIterator &di )
{
        unsigned int msg_type = get_msg_type();

        switch ( msg_type )
        {
        case STATESERVER_OBJECT_ENTER_AI_WITH_REQUIRED:
        case STATESERVER_OBJECT_ENTER_AI_WITH_REQUIRED_OTHER:
                HandleObjEntry( di, msg_type == STATESERVER_OBJECT_ENTER_AI_WITH_REQUIRED_OTHER );
                break;
        case STATESERVER_OBJECT_CHANGING_AI:
        case STATESERVER_OBJECT_DELETE_RAM:
                HandleObjExit( di );
                break;
        case STATESERVER_OBJECT_CHANGING_LOCATION:
                HandleObjLocation( di );
                break;
        case DBSERVER_CREATE_OBJECT_RESP:
        case DBSERVER_OBJECT_GET_ALL_RESP:
        case DBSERVER_OBJECT_GET_FIELDS_RESP:
        case DBSERVER_OBJECT_GET_FIELD_RESP:
        case DBSERVER_OBJECT_SET_FIELD_IF_EQUALS_RESP:
        case DBSERVER_OBJECT_SET_FIELDS_IF_EQUALS_RESP:
                m_dbInterface.HandleDatagram( msg_type, di );
                break;
        case DBSS_OBJECT_GET_ACTIVATED_RESP:
                HandleGetActivatedResp( di );
                break;
        case STATESERVER_OBJECT_SET_FIELD:
        case CLIENT_OBJECT_SET_FIELD:
                cout << "Handling stateserver/client_object_set_field..." << endl;
                ConnectionRepository::HandleDatagram( di );
                break;
        default:
                astronInternalRepository_cat.warning()
                                << "Received message with unknown MsgType " << msg_type << endl;
                break;
        }

        if ( msg_type >= 20000 )
        {
                //_net_messenger.handle(msg_type, di);
        }
}

void AstronInternalRepository::HandleObjLocation( DatagramIterator &di )
{
        DOID_TYPE doid = di.get_uint32();
        DOID_TYPE parentid = di.get_uint32();
        ZONEID_TYPE zoneid = di.get_uint32();

        DistributedObjectBase *dobj = m_mDoId2Do[doid];
        dobj->SetLocation( parentid, zoneid );
}

void AstronInternalRepository::HandleObjEntry( DatagramIterator &di, bool other )
{
        DOID_TYPE doid = di.get_uint32();
        DOID_TYPE parentid = di.get_uint32();
        ZONEID_TYPE zoneid = di.get_uint32();
        uint16_t classid = di.get_uint16();

        DCClassPP *dclass = m_mDClassByNumber[classid];
        nassertv( dclass != NULL );

        DistributedObjectBase *dobj = dclass->GetObjSingleton()->MakeNew();
        dobj->SetDClass( dclass );
        dobj->SetDoId( doid );
        AddDoToTables( dobj, parentid, zoneid );
        dobj->Generate();
        if ( other )
        {
                // all required?
                dobj->UpdateRequiredOtherFields( dclass, di );
        }
        else
        {
                // all required?
                dobj->UpdateRequiredFields( dclass, di );
        }
}

void AstronInternalRepository::HandleObjExit( DatagramIterator &di )
{
        DOID_TYPE doid = di.get_uint32();

        DistributedObjectAI *dobj = DCAST( DistributedObjectAI, m_mDoId2Do[doid] );
        RemoveDoFromTables( dobj );
        dobj->SendDeleteEvent();
        dobj->DeleteDo();
}

void AstronInternalRepository::HandleGetActivatedResp( DatagramIterator &di )
{
        uint32_t ctx = di.get_uint32();
        DOID_TYPE doid = di.get_uint32();
        bool activated = di.get_uint8() != 0;

        if ( m_mCallbacks.find( ctx ) != m_mCallbacks.end() )
        {
                GetActivatedRespClbk *clbk = m_mCallbacks[ctx];
                if ( clbk != NULL )
                {
                        ( *clbk )( doid, activated );
                }
                m_mCallbacks.erase( m_mCallbacks.find( ctx ) );
        }

}

void AstronInternalRepository::GetActivated( DOID_TYPE doid, AstronInternalRepository::GetActivatedRespClbk *clbk )
{
        uint32_t ctx = GetContext();
        m_mCallbacks[ctx] = clbk;

        Datagram dg;
        DgAddServerHeader( dg, doid, m_ourChannel, DBSS_OBJECT_GET_ACTIVATED );
        dg.add_uint32( ctx );
        dg.add_uint32( doid );
        Send( dg );
}

void AstronInternalRepository::SendActivate( DOID_TYPE doid, DOID_TYPE parentid, ZONEID_TYPE zoneid )
{
        Datagram dg;
        DgAddServerHeader( dg, doid, m_ourChannel, DBSS_OBJECT_ACTIVATE_WITH_DEFAULTS );
        dg.add_uint32( doid );
        dg.add_uint32( parentid );
        dg.add_uint32( zoneid );
        Send( dg );
}

void AstronInternalRepository::SendSetLocation( DistributedObjectBase *dobj, DOID_TYPE parentid, ZONEID_TYPE zoneid )
{
        Datagram dg;
        DgAddServerHeader( dg, dobj->GetDoId(), m_ourChannel, STATESERVER_OBJECT_SET_LOCATION );
        dg.add_uint32( parentid );
        dg.add_uint32( zoneid );
        Send( dg );
}

void AstronInternalRepository::GenerateWithRequired( DistributedObjectBase *dobj, DOID_TYPE parentid, ZONEID_TYPE zone )
{
        DOID_TYPE doId = AllocateChannel();
        GenerateWithRequiredAndId( dobj, doId, parentid, zone );
}

void AstronInternalRepository::GenerateWithRequiredAndId( DistributedObjectBase *dobj, DOID_TYPE doid, DOID_TYPE parentid, ZONEID_TYPE zone )
{
        dobj->SetDoId( doid );
        AddDoToTables( dobj, parentid, zone );

        DistributedObjectAI *dobj_ai;
        DCAST_INTO_V( dobj_ai, dobj );
        dobj_ai->SendGenerateWithRequired( parentid, zone );
}

void AstronInternalRepository::RequestDelete( DistributedObjectBase *dobj )
{
        Datagram dg;
        DgAddServerHeader( dg, dobj->GetDoId(), m_ourChannel, STATESERVER_OBJECT_DELETE_RAM );
        dg.add_uint32( dobj->GetDoId() );
        Send( dg );
}

void AstronInternalRepository::Connect( const string &host, unsigned int port )
{
        URLSpec url;
        url.set_server( host );
        url.set_port( port );

        vector<URLSpec> servs;
        servs.push_back( url );

        astronInternalRepository_cat.info()
                        << "Now connecting to " << host << ":" << port << "..." << endl;
        ConnectionRepository::Connect( servs, ConnectCallback, ConnectFailCallback, this );
}

void AstronInternalRepository::ConnectCallback( void *data )
{
        AstronInternalRepository *repo = ( AstronInternalRepository * )data;
        // We connected successfully.
        astronInternalRepository_cat.info()
                        << "Connected successfully.\n";

        // Listen to our channel...
        repo->RegisterForChannel( repo->m_ourChannel );

        Datagram dg;
        DgAddServerHeader( dg, repo->m_serverId, repo->m_ourChannel, STATESERVER_DELETE_AI_OBJECTS );
        dg.add_uint64( repo->m_ourChannel );
        repo->AddPostRemove( dg );

        throw_event( "airConnected" );
        repo->HandleConnected();
}

void AstronInternalRepository::ConnectFailCallback( void *data, int code, const string &reason, const string &host, int port )
{
        AstronInternalRepository *repo = ( AstronInternalRepository * )data;
        astronInternalRepository_cat.warning()
                        << "Failed to connect! (code=" << code << "; " << reason << ")\n";
        repo->Connect( host, port );
}

void AstronInternalRepository::HandleConnected()
{
        // Should be overriden by inheritors
}

void AstronInternalRepository::LostConnection()
{
        astronInternalRepository_cat.error()
                        << "Lost connection to gameserver!\n";
}

void AstronInternalRepository::SetEventLogHost( const string &host, unsigned int port )
{
        astronInternalRepository_cat.error()
                        << "No event logger support.\n";
}

void AstronInternalRepository::SetAI( DOID_TYPE doid, CHANNEL_TYPE aichan )
{
        Datagram dg;
        DgAddServerHeader( dg, doid, aichan, STATESERVER_OBJECT_SET_AI );
        dg.add_uint64( aichan );
        Send( dg );
}

void AstronInternalRepository::Eject( CHANNEL_TYPE clchan, int reason, const string &reasont )
{
        Datagram dg;
        DgAddServerHeader( dg, clchan, m_ourChannel, CLIENTAGENT_EJECT );
        dg.add_uint16( reason );
        dg.add_string( reasont );
        Send( dg );
}

void AstronInternalRepository::SetClientState( CHANNEL_TYPE clchan, int state )
{
        Datagram dg;
        DgAddServerHeader( dg, clchan, m_ourChannel, CLIENTAGENT_SET_STATE );
        dg.add_uint16( state );
        Send( dg );
}

void AstronInternalRepository::ClientAddSessionObject( CHANNEL_TYPE clchan, DOID_TYPE doid )
{
        Datagram dg;
        DgAddServerHeader( dg, clchan, m_ourChannel, CLIENTAGENT_ADD_SESSION_OBJECT );
        dg.add_uint32( doid );
        Send( dg );
}

void AstronInternalRepository::ClientAddInterest( CHANNEL_TYPE clchan, uint16_t interestid, DOID_TYPE parentid, ZONEID_TYPE zoneid )
{
        Datagram dg;
        DgAddServerHeader( dg, clchan, m_ourChannel, CLIENTAGENT_ADD_INTEREST );
        dg.add_uint16( interestid );
        dg.add_uint32( parentid );
        dg.add_uint32( zoneid );
        Send( dg );
}

void AstronInternalRepository::SetOwner( DOID_TYPE doid, CHANNEL_TYPE newowner )
{
        Datagram dg;
        DgAddServerHeader( dg, doid, m_ourChannel, STATESERVER_OBJECT_SET_OWNER );
        dg.add_uint64( newowner );
        Send( dg );
}