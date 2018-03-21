/**
* PANDA PLUS LIBRARY
* Copyright (c) 2017 Brian Lach <lachb@cat.pcsb.org>
*
* @file astronInternalRepository.h
* @author Brian Lach
* @date 2017-02-10
*
* @purpose Base server side repository for use with the Astron cluster.
*/

#ifndef ASTRONINTERNALREPOSITORY_H
#define ASTRONINTERNALREPOSITORY_H

#include "connectionRepository.h"
#include "dcClass_pp.h"
#include "pp_dcbase.h"
#include <pvector.h>
#include <uniqueIdAllocator.h>
#include <pmap.h>
#include "distributedObjectBase.h"
#include <genericAsyncTask.h>
#include "aiRepositoryBase.h"
#include "astronDatabaseInterface.h"

#include "config_ppdistributed.h"

NotifyCategoryDeclNoExport( astronInternalRepository );

/**
* Base server side repository for use with the astron cluster.
*/
class EXPCL_PPDISTRIBUTED AstronInternalRepository : public AIRepositoryBase
{
public:

        typedef void GetActivatedRespClbk( DOID_TYPE, bool );

        AstronInternalRepository( CHANNEL_TYPE base_channel, CHANNEL_TYPE server_id = UINT_LEAST64_MAX,
                                  const string &dc_suffix = "AI", ConnectionRepository::ConnectMethod cm = CM_HTTP,
                                  vector<string> &dc_files = vector<string>() );

        int GetContext();

        virtual void RegisterForChannel( CHANNEL_TYPE chan );
        virtual void UnregisterForChannel( CHANNEL_TYPE chan );

        void AddPostRemove( Datagram &dg );
        void ClearPostRemove();

        void HandleDatagram( DatagramIterator &di );

        void HandleObjLocation( DatagramIterator &di );
        void HandleObjEntry( DatagramIterator &di, bool other );
        void HandleObjExit( DatagramIterator &di );
        void HandleGetActivatedResp( DatagramIterator &di );
        void GetActivated( DOID_TYPE doid, GetActivatedRespClbk *clbk );
        void SendActivate( DOID_TYPE doid, DOID_TYPE parentid, ZONEID_TYPE zoneid );
        void SendSetLocation( DistributedObjectBase *dobj, DOID_TYPE parentid, ZONEID_TYPE zoneid );
        void GenerateWithRequired( DistributedObjectBase *dobj, DOID_TYPE parentid, ZONEID_TYPE zoneid );
        void GenerateWithRequiredAndId( DistributedObjectBase *dobj, DOID_TYPE doid, DOID_TYPE parentid, ZONEID_TYPE zoneid );
        void RequestDelete( DistributedObjectBase *dobj );
        void Connect( const string &host, unsigned int port = 7199 );

        virtual void HandleConnected();

        void LostConnection();

        void SetEventLogHost( const string &host, unsigned int port = 7197 );

        //write_server_event();

        void SetAI( DOID_TYPE doid, CHANNEL_TYPE aichan );

        void Eject( CHANNEL_TYPE clchan, int reason, const string &reasont );

        void SetClientState( CHANNEL_TYPE clchan, int state );

        void ClientAddSessionObject( CHANNEL_TYPE clchan, DOID_TYPE doid );
        void ClientAddInterest( CHANNEL_TYPE clchan, uint16_t interest_id, DOID_TYPE parentid, ZONEID_TYPE zoneid );

        void SetOwner( DOID_TYPE do_id, CHANNEL_TYPE newowner );

        AstronDatabaseInterface &GetDBInterface();

        friend class AstronDatabaseInterface;

private:
        static void ConnectCallback( void *data );
        static void ConnectFailCallback( void *data, int code, const string &reason, const string &host, int port );

private:
        int m_iContextCounter;
        pvector<CHANNEL_TYPE> m_vRegisteredChannels;
        pmap<uint16_t, GetActivatedRespClbk *> m_mCallbacks;

        PT( GenericAsyncTask ) m_pRetryConnectTask;

        AstronDatabaseInterface m_dbInterface;
};

#endif // ASTRONINTERNALREPOSITORY_H