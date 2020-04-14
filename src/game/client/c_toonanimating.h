#pragma once

#include "baseanimating_shared.h"
#include "cio/toondna.h"

class C_ToonAnimating : public C_BaseAnimating
{
	DECLARE_CLIENTCLASS( C_ToonAnimating, C_BaseAnimating )
public:
	C_ToonAnimating();

	void set_dna( const CToonDNA &dna );

	void on_dna_changed();

private:
	CToonDNA _dna;
};