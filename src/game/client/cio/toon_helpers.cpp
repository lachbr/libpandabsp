#include "toon_helpers.h"

#include "loader.h"

NodePath toon_legs_generate( const CToonDNA &dna )
{
	Loader *loader = Loader::get_global_ptr();

	char model_path[100];
	sprintf( model_path, "phase_3/models/char/tt_a_chr_%s_1000.bam",
		 CToonDNA::model_for_legs( dna.legs ) );

	NodePath legs_mdl( loader->load_sync( model_path ) );
	nassertr( !legs_mdl.is_empty(), NodePath() );

	LColorf legs_color = CToonDNA::body_colors[dna.legscolor];
	legs_mdl.find_all_matches( "**/legs" ).set_color( legs_color );
	legs_mdl.find_all_matches( "**/feet" ).set_color( legs_color );

	legs_mdl.find( "**/boots_long" ).stash();
	legs_mdl.find( "**/boots_short" ).stash();
	legs_mdl.find( "**/shoes" ).stash();

	return legs_mdl;
}

NodePath toon_torso_generate( const CToonDNA &dna )
{
	Loader *loader = Loader::get_global_ptr();

	char model_path[100];
	sprintf( model_path, "phase_3/models/char/tt_a_chr_%s_1000.bam",
		 CToonDNA::model_for_gender_torso( dna.gender, dna.torso ) );

	NodePath torso_mdl( loader->load_sync( model_path ) );
	nassertr( !torso_mdl.is_empty(), NodePath() );

	LColorf torso_color = CToonDNA::body_colors[dna.torsocolor];
	LColorf glove_color = CToonDNA::clothes_colors[dna.glovecolor];
	torso_mdl.find_all_matches( "**/arms" ).set_color( torso_color );
	torso_mdl.find_all_matches( "**/neck" ).set_color( torso_color );
	torso_mdl.find_all_matches( "**/hands" ).set_color( glove_color );

	return torso_mdl;
}
