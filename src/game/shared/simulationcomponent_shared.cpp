#include "simulationcomponent_shared.h"

bool SimulationComponent::initialize()
{
	if ( !BaseClass::initialize() )
	{
		return false;
	}

	simulation_time = 0.0f;
	simulation_tick = 0;
#ifdef CLIENT_DLL
	old_simulation_time = 0.0f;
#endif

	return true;
}

#if !defined( CLIENT_DLL )

//#include "baseentity_shared.h"
#include "serverbase.h"

void CSimulationComponent::update( double frametime )
{
	simulation_time = sv->get_curtime();
}

void SendProxy_SimulationTime( SendProp *prop, void *object, void *data, Datagram &out )
{
	CSimulationComponent *ent = (CSimulationComponent *)object;

	int ticknumber = sv->time_to_ticks( ent->simulation_time );
	// tickbase is current tick rounded down to closest 100 ticks
	int tickbase =
		sv->get_network_base( sv->get_tickcount(), ent->get_entity()->get_entnum() );
	int addt = 0;
	if ( ticknumber >= tickbase )
	{
		addt = ( ticknumber - tickbase ) & 0xFF;
	}

	out.add_int32( addt );
}

IMPLEMENT_SERVERCLASS_ST_NOBASE( CSimulationComponent )
	new SendProp( PROPINFO( simulation_time ), 0, SendProxy_SimulationTime ),
	SendPropInt( PROPINFO( simulation_tick ) ),
END_SEND_TABLE()

#else

#include "clientbase.h"

void RecvProxy_SimulationTime( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	C_SimulationComponent *ent = (C_SimulationComponent *)object;

	int t;
	int tickbase;
	int addt;

	// Unpack the data.
	addt = dgi.get_int32();

	// Note, this needs to be encoded relative to the packet timestamp, not raw client
	// clock
	tickbase = cl->get_network_base( cl->get_tickcount(), ent->get_entity()->get_entnum() );

	t = tickbase;
	// and then go back to floating point time
	t += addt; // Add in an additional up to 256 100ths from the server.

	// center _simulation_time around current time.
	while ( t < cl->get_tickcount() - 127 )
		t += 256;
	while ( t > cl->get_tickcount() + 127 )
		t -= 256;

	ent->simulation_time = t * cl->get_interval_per_tick();

	//c_baseentity_cat.debug()
	//	<< "Received simulation time. Server time: " << addt
	//	<< ", Simulation time: " << ent->_simulation_time << "\n";
}

IMPLEMENT_CLIENTCLASS_RT_NOBASE( C_SimulationComponent, CSimulationComponent )
	RecvPropInt( PROPINFO( simulation_time ), RecvProxy_SimulationTime ),
	RecvPropInt( PROPINFO( simulation_tick ) ),
END_RECV_TABLE()

#endif