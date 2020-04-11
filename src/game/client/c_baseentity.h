/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file c_baseentity.h
 * @author Brian Lach
 * @date January 25, 2020
 */

#ifndef C_BASEENTITY_H_
#define C_BASEENTITY_H_

#include <typedReferenceCount.h>

#include "config_clientdll.h"


#include <aa_luse.h>
#include <nodePath.h>
#include <pvector.h>

#include "client_class.h"

NotifyCategoryDeclNoExport( c_baseentity )

class EXPORT_CLIENT_DLL C_BaseEntity : public CBaseEntityShared
{
	DECLARE_CLASS( C_BaseEntity, CBaseEntityShared )

public:
	C_BaseEntity();

	entid_t get_owner() const;

	void send_entity_message( Datagram &dg );

	virtual void receive_entity_message( int msgtype, DatagramIterator &dgi );

	virtual void post_data_update();

	virtual void spawn();
	virtual void init( entid_t entnum );

public:
	entid_t _owner_entity;
};

INLINE entid_t C_BaseEntity::get_owner() const
{
	return _owner_entity;
}

#endif // C_BASEENTITY_H_
