#include "toonanimating.h"

bool CToonAnimating::initialize()
{
	if ( !BaseClass::initialize() )
	{
		return false;
	}

	//_dna = CToonDNA();

	return true;
}
void CToonAnimating::set_dna( const CToonDNA &dna )
{
	_dna = dna;
}

CToonDNA CToonAnimating::get_dna() const
{
	return _dna;
}

static void SendProxy_DNA( SendProp *prop, void *object, void *value, Datagram &dg )
{
	CToonDNA *dna = (CToonDNA *)value;
	dna->write_datagram( dg );
}

IMPLEMENT_SERVERCLASS_ST( CToonAnimating )
	new SendProp( PROPINFO( _dna ), 0, SendProxy_DNA )
END_SEND_TABLE()
