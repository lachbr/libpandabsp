#include "toon_cage.h"

#include "baseanimating_shared.h"

void CToonCage::add_components()
{
	BaseClass::add_components();

	PT( CBaseAnimating ) animating = new CBaseAnimating;
	add_component( animating );
}

void CToonCage::spawn()
{
	BaseClass::spawn();

	CBaseAnimating *animating;
	get_component( animating );
	animating->set_model( "models/cage.bam" );
}

IMPLEMENT_ENTITY( CToonCage, cog_cage )