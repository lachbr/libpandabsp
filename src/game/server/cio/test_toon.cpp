#include "baseentity.h"
#include "toonanimating.h"
#include "scenecomponent_shared.h"

class CTestToon : public CBaseEntity
{
	DECLARE_ENTITY( CTestToon, CBaseEntity )
public:
	virtual void add_components();

	virtual void spawn();

	CToonAnimating *toon;
};

void CTestToon::spawn()
{
	BaseClass::spawn();

	_scene->set_origin( LPoint3f( 0, 25, 0 ) );
}

void CTestToon::add_components()
{
	BaseClass::add_components();

	CToonDNA dna;
	dna.animal = CToonDNA::ANIMAL_DOG;
	dna.gender = CToonDNA::GENDER_BOY;
	dna.head = CToonDNA::HEAD_SHORTSHORT;
	dna.headcolor = CToonDNA::BODYCOLOR_AQUA;
	dna.torso = CToonDNA::TORSO_MEDIUM;
	dna.torsocolor = CToonDNA::BODYCOLOR_AQUA;
	dna.legs = CToonDNA::LEGS_MEDIUM;
	dna.legscolor = CToonDNA::BODYCOLOR_AQUA;
	PT( CToonAnimating ) t = new CToonAnimating;
	t->set_dna( dna );
	add_component( t );
	toon = t;
}

IMPLEMENT_ENTITY( CTestToon, test_toon )