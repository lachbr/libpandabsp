#include "bsploader.h"

#include <load_prc_file.h>
#include <pandaFramework.h>
#include <ambientLight.h>
#include <directionalLight.h>
#include <shadeModelAttrib.h>
#include <renderModeAttrib.h>
#include <pstatClient.h>
#include <virtualFileSimple.h>

struct Vector
{
	int x, y, z, w;
};

int main( int argc, char *argv[] )
{
        VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();
        vfs->mount( Filename( "../../../../cio/game/resources/phase_0.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
        vfs->mount( Filename( "../../../../cio/game/resources/phase_3.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
        vfs->mount( Filename( "../../../../cio/game/resources/phase_3.5.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
        vfs->mount( Filename( "../../../../cio/game/resources/phase_4.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
        vfs->mount( Filename( "../../../../cio/game/resources/phase_5.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
        vfs->mount( Filename( "../../../../cio/game/resources/phase_5.5.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
        vfs->mount( Filename( "../../../../cio/game/resources/phase_6.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
        vfs->mount( Filename( "../../../../cio/game/resources/phase_7.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
        vfs->mount( Filename( "../../../../cio/game/resources/phase_8.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
        vfs->mount( Filename( "../../../../cio/game/resources/phase_9.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
        vfs->mount( Filename( "../../../../cio/game/resources/phase_10.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
        vfs->mount( Filename( "../../../../cio/game/resources/phase_11.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
        vfs->mount( Filename( "../../../../cio/game/resources/phase_12.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
        vfs->mount( Filename( "../../../../cio/game/resources/phase_13.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );

        load_prc_file( "../../../panda3d/built_x64/etc/Confauto.prc" );
        load_prc_file( "../../../panda3d/built_x64/etc/Config.prc" );
        load_prc_file_data( "", "notify-level-gobj fatal" );
	load_prc_file_data( "", "show-frame-rate-meter #t" );
	load_prc_file_data( "", "vfs-case-sensitive #f" );
	//load_prc_file_data( "", "load-display pandadx9" );
	//load_prc_file_data( "", "threading-model App/Cull/Draw" );

        PandaFramework framework;
        framework.open_framework( argc, argv );
        WindowFramework *window = framework.open_window();
        window->setup_trackball();
        window->set_background_type( WindowFramework::BT_black );
        window->get_camera( 0 )->get_lens()->set_min_fov( 70.0 / ( 4. / 3. ) );
        //window->get_render().set_render_mode_filled_wireframe(LColor(0, 0, 0, 1.0), 1);

        NodePath render = window->get_render();

        PStatClient::connect();

	cout << sizeof( LVector3 ) << endl;

        //NodePath( window->get_camera( 0 ) ).show_bounds();

        BSPLoader bsp_file;
        bsp_file.set_gsg( window->get_graphics_window()->get_gsg() );
        bsp_file.set_camera( NodePath( window->get_camera( 0 ) ), window->get_camera_group() );
	bsp_file.set_want_visibility( true );
	bsp_file.read( Filename::from_os_specific( argv[1] ).get_fullpath() );
	NodePath map = bsp_file.get_result();
	map.reparent_to( render );
        map.ls();
        //map.set_two_sided( true );
        //map.set_scale( -0.075, -0.075, 0.075 );
        map.set_scale( 0.075 );
	//map.ls();
        //map.flatten_medium();
        //map.set_two_sided( true );
        //map.set_attrib( ShadeModelAttrib::make( ShadeModelAttrib::M_smooth ), 1 );;
        //map.set_two_sided( true );

        //PT( AmbientLight ) amb = new AmbientLight( "amb" );
        //amb->set_color( LColor( 0.3, 0.3, 0.3, 1.0 ) );
        //PT( DirectionalLight ) dir = new DirectionalLight( "dir" );
        //dir->set_color( LColor( 1.0, 1.0, 1.0, 1.0 ) );
        //dir->set_direction( LVector3( 0.5, 0.5, -0.5 ) );
        
        //NodePath amb_np = render.attach_new_node( amb );
        //render.set_light( amb_np );
        //NodePath dir_np = render.attach_new_node( dir );
        //render.set_light( dir_np );

        //render.set_shader_auto();

        //render.set_render_mode_filled_wireframe( LColor( 1.0, 1.0, 1.0, 1.0 ), 1 );

        framework.main_loop();
        return 0;
}