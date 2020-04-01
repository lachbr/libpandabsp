#include <basegame.h>
#include <load_prc_file.h>
#include <baseanimating.h>
#include <entregistry.h>
#include <globalvars_server.h>

class CIGame : public BaseGame
{
protected:
	virtual void mount_multifiles();
};

void CIGame::mount_multifiles()
{
	BaseGame::mount_multifiles();

	mount_multifile( "resources/phase_0.mf" );
	mount_multifile( "resources/phase_3.mf" );
	mount_multifile( "resources/phase_3.5.mf" );
	mount_multifile( "resources/phase_4.mf" );
	mount_multifile( "resources/phase_5.mf" );
	mount_multifile( "resources/phase_5.5.mf" );
	mount_multifile( "resources/phase_6.mf" );
	mount_multifile( "resources/phase_7.mf" );
	mount_multifile( "resources/phase_8.mf" );
	mount_multifile( "resources/phase_9.mf" );
	mount_multifile( "resources/phase_10.mf" );
	mount_multifile( "resources/phase_11.mf" );
	mount_multifile( "resources/phase_12.mf" );
	mount_multifile( "resources/phase_13.mf" );
	mount_multifile( "resources/phase_14.mf" );
	mount_multifile( "resources/mod.mf" );
}

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
	virtual void spawn();
	virtual void think();

private:
	float _last_change;
	int _anim;
};

void CTestEnt::spawn()
{
	BaseClass::spawn();
	set_model( "models/health_charger/health_charger.act" );
	set_origin( LPoint3( 12, 75, 5 ) );
	set_animation( "idle" );
}

void CTestEnt::think()
{
	if ( g_globals->curtime - _last_change >= 1.0f )
	{
		_anim++;
		if ( _anim >= 3 )
			_anim = 0;
		set_animation( anims[_anim] );
		_last_change = g_globals->curtime;
	}

	set_origin( LPoint3( 12, 75, 5 + std::sin( g_globals->curtime ) ) );
}

IMPLEMENT_SERVERCLASS_ST(CTestEnt, DT_TestEnt, testent)
END_SEND_TABLE()

int main( int argc, char **argv )
{
	load_prc_file_data( "", "notify-level debug" );
	load_prc_file_data( "", "support-ipv6 0" );

	CEntRegistry::ptr()->init_server_classes();
	CTestEnt::server_class_init();

	PT( CIGame ) game = new CIGame;
	game->startup();

	CreateEntityByName( "testent" );

	for ( int i = 0; i < argc; i++ )
	{
		const char *cmd = argv[i];
		if ( !strncmp( cmd, "+map", 4 ) )
		{
			if ( i < argc - 1 )
			{
				const char *mapname = argv[++i];
				game->load_bsp_level( game->get_map_filename( mapname ) );
			}
			else
			{
				std::cout << "Error: got +map but no map name" << std::endl;
				return 1;
			}
		}
	}

	game->run();

	return 0;
}