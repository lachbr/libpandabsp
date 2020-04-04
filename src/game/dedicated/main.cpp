#include "serverbase.h"
#include "sv_bsploader.h"
#include "baseanimating.h"
#include "entregistry.h"

#include <randomizer.h>

std::string anims[] =
{
	"idle",
	"empty",
	"emptyclick",
};

class CTestEnt : public CBaseAnimating
{
	DECLARE_CLASS( CTestEnt, CBaseAnimating )
	DECLARE_SERVERCLASS()

public:
	CTestEnt() :
		_last_change( 0.0f ),
		_anim( 0 )
	{
	}
	virtual void spawn();
	virtual void think();

	virtual void receive_entity_message( int msgtype, uint32_t client, DatagramIterator &dgi );

private:
	float _last_change;
	int _anim;
};

void CTestEnt::receive_entity_message( int msgtype, uint32_t client, DatagramIterator &dgi )
{
	switch ( msgtype )
	{
	case 0:
		{
			std::string msg = dgi.get_string();
			std::cout << "CTestEnt: Client said hello! " << msg << std::endl;

			Datagram dg;
			begin_entity_message( dg, 1 );
			dg.add_string( "HELLO FROM SERVER CTestEnt!" );
			send_entity_message( dg, { client } );
			break;
		}
	}
}

void CTestEnt::spawn()
{
	BaseClass::spawn();
	//set_model( "models/health_charger/health_charger.act" );
	set_model( "phase_14/models/props/creampie.bam" );
	set_mass( 5.0f );
	set_solid( SOLID_MESH );
	init_physics();
	set_origin( LPoint3( 12, 100, 5 ) );
	//set_animation( "idle" );
}

void CTestEnt::think()
{
	if ( sv->get_curtime() - _last_change >= 5.0f )
	{
		Randomizer random;
		//_anim++;
		//if ( _anim >= 3 )
		//	_anim = 0;
		//set_animation( anims[_anim] );
		_last_change = sv->get_curtime();
		get_physics_node()->set_active( true, true );
		get_physics_node()->set_linear_velocity( LVector3( 0, 0, 12.5 ) );
		get_physics_node()->set_angular_velocity( LVector3( random.random_real( 2 ) - 1, random.random_real( 2 ) - 1, random.random_real( 2 ) - 1 ) * 5 );
		//get_physics_node()->apply_impulse( LVector3( 0, 0, 500 ), LVector3( 0, 0, 0 ) );
	}

	//set_origin( LPoint3( 12, 75, 5 + std::sin( sv->get_curtime() ) ) );
	
}

IMPLEMENT_SERVERCLASS_ST( CTestEnt, DT_TestEnt, testent )
END_SEND_TABLE()

int main( int argc, char **argv )
{
	PT( ServerBase ) sv = new ServerBase;
	sv->startup();

	CTestEnt::server_class_init();

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

	CreateEntityByName( "testent" );

	sv->run();
	return 0;
}