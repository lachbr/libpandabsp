#include <c_basegame.h>
#include <c_entregistry.h>
#include <c_baseanimating.h>

#include <load_prc_file.h>

class C_CIGame : public C_BaseGame
{
protected:
	virtual void mount_multifiles();
};

void C_CIGame::mount_multifiles()
{
	C_BaseGame::mount_multifiles();

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

class C_TestEnt : public C_BaseAnimating
{
	DECLARE_CLASS( C_TestEnt, C_BaseAnimating )
	DECLARE_CLIENTCLASS()
};

IMPLEMENT_CLIENTCLASS_RT(C_TestEnt, DT_TestEnt, CTestEnt)
END_RECV_TABLE()

int main( int argc, char **argv )
{
	load_prc_file_data( "", "notify-level debug" );
	load_prc_file_data( "", "support-ipv6 0" );

	C_EntRegistry::ptr()->init_client_classes();
	C_TestEnt::client_class_init();

	PT( C_CIGame ) game = new C_CIGame;
	game->startup();
	game->_camera.set_pos( 10, 70, 5 );

	game->run_cmd( "map facility" );

	game->run();

	return 0;
}