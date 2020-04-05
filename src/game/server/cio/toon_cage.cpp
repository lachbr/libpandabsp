#include "toon_cage.h"

void CToonCage::spawn()
{
	BaseClass::spawn();

	set_model( "models/cage.bam" );

	LVector3f origin = get_entity_value_vector( "origin" ) / 16;
	LVector3f angles = get_entity_value_vector( "angles" );
	set_origin( origin );
	set_angles( LVector3f( angles[1] - 90, angles[0], angles[2] ) );
}

IMPLEMENT_SERVERCLASS_ST( CToonCage )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( CToonCage, cog_cage )