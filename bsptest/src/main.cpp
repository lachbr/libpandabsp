#include "bspfile.h"

#include <load_prc_file.h>
#include <pandaFramework.h>
#include <ambientLight.h>
#include <directionalLight.h>
#include <shadeModelAttrib.h>
#include <renderModeAttrib.h>
#include <pstatClient.h>

int main( int argc, char *argv[] )
{
        load_prc_file( "../../../panda3d/built_x64/etc/Confauto.prc" );
        load_prc_file( "../../../panda3d/built_x64/etc/Config.prc" );
        load_prc_file_data( "", "notify-level-gobj fatal" );

        PandaFramework framework;
        framework.open_framework( argc, argv );
        WindowFramework *window = framework.open_window();
        window->setup_trackball();
        window->set_background_type( WindowFramework::BT_black );
        window->get_camera( 0 )->get_lens()->set_min_fov( 70.0 / ( 4. / 3. ) );

        NodePath render = window->get_render();

        PStatClient::connect();

        BSPFile bsp_file;
        bsp_file.set_camera( NodePath( window->get_camera( 0 ) ), window->get_camera_group() );
        bsp_file.read( Filename( argv[1] ) );
        NodePath map = window->get_render().attach_new_node( bsp_file.get_result() );
        //map.set_scale( -0.075, -0.075, 0.075 );
        map.set_scale( 0.075 );
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

        render.set_shader_auto();

        //render.set_render_mode_filled_wireframe( LColor( 1.0, 1.0, 1.0, 1.0 ), 1 );

        framework.main_loop();
        return 0;
}