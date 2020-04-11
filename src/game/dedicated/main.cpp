#include "serverbase.h"
#include "sv_bsploader.h"
#include "baseanimating_shared.h"
#include "physicscomponent_shared.h"
#include "scenecomponent_shared.h"

#include <randomizer.h>

std::string anims[] =
{
	"idle",
	"empty",
	"emptyclick",
};

class CTestEnt : public CBaseEntity
{
	DECLARE_ENTITY( CTestEnt, CBaseEntity )

public:
	CTestEnt() :
		_last_change( 0.0f ),
		_anim( 0 )
	{
	}

	virtual void add_components()
	{
		BaseClass::add_components();

		PT( CBaseAnimating ) anim = new CBaseAnimating;
		add_component( anim );
		animating = anim;

		PT( CPhysicsComponent ) phys = new CPhysicsComponent;
		add_component( phys );
		physics = phys;
	}
	virtual void spawn();
	virtual void simulate( double frametime );

private:
	float _last_change;
	int _anim;

	CBaseAnimating *animating;
	CPhysicsComponent *physics;
};

void CTestEnt::spawn()
{
	physics->mass = 1.0f;
	physics->solid = CPhysicsComponent::SOLID_MESH;
	physics->kinematic = true;

	animating->set_model( "phase_14/models/props/creampie.bam" );

	BaseClass::spawn();
	//set_model( "models/health_charger/health_charger.act" );
	
	
	_scene->set_origin( LPoint3( 40 / 16.0f, 650 / 16.0f, 8 / 16.0f ) );
	//set_animation( "idle" );
}

void CTestEnt::simulate( double frametime )
{
	if ( sv->get_curtime() - _last_change >= 5.0f )
	{
		Randomizer random;
		//_anim++;
		//if ( _anim >= 3 )
		//	_anim = 0;
		//set_animation( anims[_anim] );
		_last_change = sv->get_curtime();
		physics->bodynode->set_active( true, true );
		physics->bodynode->set_linear_velocity( LVector3( 0, 0, 12.5 ) );
		physics->bodynode->set_angular_velocity( LVector3( random.random_real( 2 ) - 1, random.random_real( 2 ) - 1, random.random_real( 2 ) - 1 ) * 5 );
		//get_physics_node()->apply_impulse( LVector3( 0, 0, 500 ), LVector3( 0, 0, 0 ) );
	}

	//set_origin( LPoint3( 12, 75, 5 + std::sin( sv->get_curtime() ) ) );

	BaseClass::simulate( frametime );
	
}

IMPLEMENT_ENTITY( CTestEnt, test_ent )

int main( int argc, char **argv )
{
	PT( ServerBase ) sv = new ServerBase;
	sv->startup();

	for ( int i = 0; i < argc; i++ )
	{
		const char *cmd = argv[i];
		if ( !strncmp( cmd, "+map", 4 ) )
		{
			if ( i < argc - 1 )
			{
				const char *mapname = argv[++i];
				svbsp->load_bsp_level( svbsp->get_map_filename( mapname ) );
			}
			else
			{
				std::cout << "Error: got +map but no map name" << std::endl;
				return 1;
			}
		}
	}

	CreateEntityByName( "test_ent" );

	sv->get_root().ls();

	sv->run();
	return 0;
}