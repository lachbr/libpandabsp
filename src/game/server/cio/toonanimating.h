#pragma once

#include "baseanimating_shared.h"
#include "cio/toondna.h"

class CToonAnimating : public CBaseAnimating
{
	DECLARE_SERVERCLASS( CToonAnimating, CBaseAnimating )
public:
	virtual bool initialize();
	void set_dna( const CToonDNA &dna );
	CToonDNA get_dna() const;
	NetworkVar( CToonDNA, _dna );
};