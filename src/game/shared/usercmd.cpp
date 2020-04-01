#include "usercmd.h"

CUserCmd::CUserCmd()
{
	clear();
}

void CUserCmd::clear()
{
	tickcount = 0;
	commandnumber = 0;
	forwardmove = 0;
	sidemove = 0;
	upmove = 0;
	buttons = 0;
	weaponselect = 0;
	weaponsubtype = 0;
	mousedx = 0;
	hasbeenpredicted = 0;
	tickcount = 0;
}

void CUserCmd::read_datagram( DatagramIterator &dgi, CUserCmd *from )
{
	// assume no change
	*this = *from;

	if ( dgi.get_uint8() )
	{
		commandnumber = dgi.get_uint32();
	}
	else
	{
		// Assume steady increment
		commandnumber = from->commandnumber + 1;
	}

	if ( dgi.get_uint8() )
	{
		tickcount = dgi.get_uint32();
	}
	else
	{
		// Assume steady increment
		tickcount = from->tickcount + 1;
	}

	if ( dgi.get_uint8() )
	{
		viewangles.read_datagram_fixed( dgi );
	}

	if ( dgi.get_uint8() )
	{
		forwardmove = dgi.get_float32();
	}

	if ( dgi.get_uint8() )
	{
		sidemove = dgi.get_float32();
	}

	if ( dgi.get_uint8() )
	{
		upmove = dgi.get_float32();
	}

	if ( dgi.get_uint8() )
	{
		buttons = dgi.get_uint32();
	}

	if ( dgi.get_uint8() )
	{
		weaponselect = dgi.get_uint8();
		if ( dgi.get_uint8() )
		{
			weaponsubtype = dgi.get_uint8();
		}
	}

	if ( dgi.get_uint8() )
	{
		mousedx = dgi.get_int16();
	}

	if ( dgi.get_uint8() )
	{
		mousedy = dgi.get_int16();
	}
}

void CUserCmd::write_datagram( Datagram &dg, CUserCmd *from )
{
	if ( commandnumber != ( from->commandnumber + 1 ) )
	{
		dg.add_uint8( 1 );
		dg.add_uint32( tickcount );
	}
	else
	{
		dg.add_uint8( 0 );
	}

	if ( tickcount != ( from->tickcount + 1 ) )
	{
		dg.add_uint8( 1 );
		dg.add_uint32( tickcount );
	}
	else
	{
		dg.add_uint8( 0 );
	}

	if ( viewangles != from->viewangles )
	{
		dg.add_uint8( 1 );
		viewangles.write_datagram_fixed( dg );
	}
	else
	{
		dg.add_uint8( 0 );
	}

	if ( forwardmove != from->forwardmove )
	{
		dg.add_uint8( 1 );
		dg.add_float32( forwardmove );
	}
	else
	{
		dg.add_uint8( 0 );
	}

	if ( sidemove != from->sidemove )
	{
		dg.add_uint8( 1 );
		dg.add_float32( sidemove );
	}
	else
	{
		dg.add_uint8( 0 );
	}

	if ( upmove != from->upmove )
	{
		dg.add_uint8( 1 );
		dg.add_float32( upmove );
	}
	else
	{
		dg.add_uint8( 0 );
	}

	if ( buttons != from->buttons )
	{
		dg.add_uint8( 1 );
		dg.add_uint32( buttons );
	}
	else
	{
		dg.add_uint8( 0 );
	}

	if ( weaponselect != from->weaponselect )
	{
		dg.add_uint8( 1 );
		dg.add_uint8( weaponselect );

		if ( weaponsubtype != from->weaponsubtype )
		{
			dg.add_uint8( 1 );
			dg.add_uint8( weaponsubtype );
		}
		else
		{
			dg.add_uint8( 0 );
		}
	}
	else
	{
		dg.add_uint8( 0 );
	}

	if ( mousedx != from->mousedx )
	{
		dg.add_uint8( 1 );
		dg.add_int16( mousedx );
	}
	else
	{
		dg.add_uint8( 0 );
	}

	if ( mousedy != from->mousedy )
	{
		dg.add_uint8( 1 );
		dg.add_int16( mousedy );
	}
	else
	{
		dg.add_uint8( 0 );
	}
}
