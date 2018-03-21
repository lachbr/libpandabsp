#include "aiRepositoryBase.h"

AIRepositoryBase::AIRepositoryBase( vector<string> &dc_files, ConnectionRepository::ConnectMethod cm )
        : m_ourChannel( 0 ), m_districtId( 0 ), m_serverId( 0 ),
          ConnectionRepository( dc_files, cm )
{
}

CHANNEL_TYPE AIRepositoryBase::AllocateChannel()
{
        return m_idAlloc.allocate();
}

void AIRepositoryBase::DeallocateChannel( CHANNEL_TYPE chan )
{
        m_idAlloc.free( chan );
}

CHANNEL_TYPE AIRepositoryBase::GetOurChannel() const
{
        return m_ourChannel;
}

CHANNEL_TYPE AIRepositoryBase::GetDistrictId() const
{
        return m_districtId;
}

CHANNEL_TYPE AIRepositoryBase::GetServerId() const
{
        return m_serverId;
}

int AIRepositoryBase::GetContext()
{
        // Might need to be overridden.
        return 0;
}

DOID_TYPE AIRepositoryBase::GetAccountIdFromSender() const
{
        return ( get_msg_sender() >> 32 ) & 0xFFFFFFFF;
}

DOID_TYPE AIRepositoryBase::GetAvatarIdFromSender() const
{
        return get_msg_sender() & 0xFFFFFFFF;
}

void AIRepositoryBase::RegisterForChannel( CHANNEL_TYPE chan )
{
}

void AIRepositoryBase::UnregisterForChannel( CHANNEL_TYPE chan )
{
}