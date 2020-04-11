#pragma once

#include "shareddefs.h"

#ifdef CLIENT_DLL
#define SimulationComponent C_SimulationComponent
#else
#define SimulationComponent CSimulationComponent
#endif

/**
 * Manages the simulation time of an entity.
 */
class EXPORT_GAME_SHARED SimulationComponent : public CBaseComponent
{
	DECLARE_COMPONENT( SimulationComponent, CBaseComponent )
public:

	float get_simulation_time() const;

	virtual bool initialize();

#if !defined( CLIENT_DLL )

	virtual void update( double frametime );

	NetworkFloat( simulation_time );
#else
	bool is_simulation_changed() const;

	float simulation_time;
	float old_simulation_time;
#endif
};

inline float SimulationComponent::get_simulation_time() const
{
	return simulation_time;
}

#ifdef CLIENT_DLL

inline bool C_SimulationComponent::is_simulation_changed() const
{
	return simulation_time != old_simulation_time;
}

#endif
