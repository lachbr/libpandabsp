/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file c_baseentity.cpp
 * @author Brian Lach
 * @date January 25, 2020
 */

#include "c_baseentity.h"
#include "clientbase.h"
#include "cl_netinterface.h"
#include "simulationcomponent_shared.h"
#include <configVariableBool.h>

#include <modelRoot.h>
#include <pStatCollector.h>
#include <pStatTimer.h>

NotifyCategoryDef( c_baseentity, "" )

IMPLEMENT_CLASS( C_BaseEntity )

C_BaseEntity::C_BaseEntity() :
	//_bsp_entnum( -1 ),
	//_simulation_tick( 0 ),
	
	_owner_entity( 0 )
{
}

void C_BaseEntity::init( entid_t entnum )
{
	BaseClass::init( entnum );
	//_entnum = entnum;

	
}

void C_BaseEntity::spawn()
{
	CBaseEntityShared::spawn();
	//update_parent_entity( get_parent_entity() );
}

void C_BaseEntity::post_data_update()
{
	// This is trash.

	C_SimulationComponent *sim;
	get_component( sim );

	bool simulation_changed = sim->simulation_time != sim->old_simulation_time;

	size_t count = _ordered_components.size();
	for ( size_t i = 0; i < count; i++ )
	{
		C_BaseComponent *comp = DCAST( C_BaseComponent, _ordered_components[i].component );

		comp->post_data_update();
		
		if ( !comp->is_predictable() )
		{
			if ( simulation_changed )
			{
				// Update interpolated simulation vars
				comp->on_latch_interpolated_vars( LATCH_SIMULATION_VAR );
			}
		}
		else
		{
			// Just store off the last networked value for use in prediction
			comp->on_store_last_networked_value();
		}

		sim->old_simulation_time = sim->simulation_time;
	}
}

void C_BaseEntity::send_entity_message( Datagram &dg )
{
	clnet->send_datagram( dg );
}

void C_BaseEntity::receive_entity_message( int msgtype, DatagramIterator &dgi )
{
}