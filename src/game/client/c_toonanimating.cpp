#include "c_toonanimating.h"
#include "cio/toon_helpers.h"

static void RecvProxy_DNA( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	C_ToonAnimating *comp = (C_ToonAnimating *)object;
	CToonDNA *dna = (CToonDNA *)out;
	dna->read_datagram( dgi );
	comp->on_dna_changed();
}

C_ToonAnimating::C_ToonAnimating() :
	C_BaseAnimating()
{
}

void C_ToonAnimating::set_dna( const CToonDNA &dna )
{
	_dna = dna;
	on_dna_changed();
}

void C_ToonAnimating::on_dna_changed()
{
	NodePath legs = toon_legs_generate( _dna );
	NodePath torso = toon_torso_generate( _dna );

	legs.reparent_to( _model_np );
	torso.reparent_to( legs.find( "**/joint_hips" ) );

	_model_np.set_scale( CToonDNA::scale_for_animal( _dna.animal ) );
}

IMPLEMENT_CLIENTCLASS_RT( C_ToonAnimating, CToonAnimating )
	new RecvProp(PROPINFO(_dna), RecvProxy_DNA)
END_RECV_TABLE()