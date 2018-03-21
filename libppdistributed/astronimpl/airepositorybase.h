/**
* PANDA PLUS LIBRARY
* Copyright (c) 2017 Brian Lach <lachb@cat.pcsb.org>
*
* @file aiRepositoryBase.h
* @author Brian Lach
* @date 2017-02-14
*
* @purpose This is an abstract class for any AI repository (including uberdogs).
*          It should be utilized by both the Astron and CMU implementations.
*/

#ifndef AIREPOSITORYBASE_H
#define AIREPOSITORYBASE_H

#include "connectionRepository.h"
#include <uniqueIdAllocator.h>

class DistributedObjectBase;

/**
* This is an abstract class for any AI repository (including uberdogs).
* It should be utilized by the Astron implementation.
* (A CMU AI repository should inherit from ClientRepository.)
*/
class EXPCL_PPDISTRIBUTED AIRepositoryBase : public ConnectionRepository
{
public:

        AIRepositoryBase( vector<string> &dc_files, ConnectionRepository::ConnectMethod cm );

        virtual int GetContext();

        DOID_TYPE GetAccountIdFromSender() const;
        virtual DOID_TYPE GetAvatarIdFromSender() const;

        //virtual void

        // pure virtual
        virtual void GenerateWithRequired( DistributedObjectBase *dobj, DOID_TYPE parentid, ZONEID_TYPE zoneid ) = 0;
        virtual void GenerateWithRequiredAndId( DistributedObjectBase *dobj, DOID_TYPE doid, DOID_TYPE parentid, ZONEID_TYPE zoneid ) = 0;

        virtual void RegisterForChannel( CHANNEL_TYPE chan );
        virtual void UnregisterForChannel( CHANNEL_TYPE chan );

        virtual CHANNEL_TYPE GetDistrictId() const;
        virtual CHANNEL_TYPE GetServerId() const;
        virtual CHANNEL_TYPE GetOurChannel() const;

        virtual CHANNEL_TYPE AllocateChannel();
        virtual void DeallocateChannel( CHANNEL_TYPE chan );

protected:
        CHANNEL_TYPE m_districtId;
        CHANNEL_TYPE m_serverId;
        CHANNEL_TYPE m_ourChannel;
        UniqueIdAllocator m_idAlloc;
};

#endif // AIREPOSITORYBASE_H