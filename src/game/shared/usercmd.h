#pragma once

#include "config_game_shared.h"

#include <referenceCount.h>
#include <aa_luse.h>
#include <datagram.h>
#include <datagramIterator.h>

class EXPORT_GAME_SHARED CUserCmd
{
public:
	CUserCmd();

	void clear();

	virtual void read_datagram( DatagramIterator &dgi, CUserCmd *from );
	virtual void write_datagram( Datagram &dg, CUserCmd *from );

public:
	uint32_t tickcount;
	uint32_t commandnumber;

	// Movement velocities
	PN_float32 forwardmove;
	PN_float32 sidemove;
	PN_float32 upmove;

	// Gameplay button states
	uint32_t buttons;

	// Player view angles
	LVector3f viewangles;
	
	// Weapon selection
	uint8_t weaponselect;
	uint8_t weaponsubtype;

	// Mouse movement deltas
	int16_t mousedx;
	int16_t mousedy;

	uint8_t hasbeenpredicted;
};