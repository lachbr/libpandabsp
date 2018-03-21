#include "map.h"
#include "door.h"
#include "pp_globals.h"
#include "mapinfo.h"
#include "devshotcamera.h"

#include <occluderNode.h>

Map::
Map() :
        NodePath( "map" ),
        _event_handler( EventHandler::get_global_event_handler() )
{
}

//void Map::
//operator =(const Map &other) {
//  NodePath::operator=(other);
//}

MapInfo *Map::
get_map_info() const
{
        NodePath mapinfo_np = find( "**/+MapInfo" );

        nassertr( !mapinfo_np.is_empty(), NULL );

        return DCAST( MapInfo, mapinfo_np.node() );
}

/**
* Enable all of the occluders present in the map.
*/
void Map::
enable_occluders()
{
        NodePathCollection occls = find_all_matches( "**/+OccluderNode" );
        for ( int i = 0; i < occls.get_num_paths(); i++ )
        {
                g_render.set_occluder( occls[i] );
        }
}

/**
* Enable the occluder with the name specified.
*/
void Map::
enable_occluder( const string &name )
{
        NodePathCollection occls = find_all_matches( "**/occluder." + name );
        for ( int i = 0; i < occls.get_num_paths(); i++ )
        {
                DCAST( OccluderNode, occls[i].node() )->set_double_sided( true );
                g_render.set_occluder( occls[i] );
        }
}

/**
* Disable the occluder with the name specified.
*/
void Map::
disable_occluder( const string &name )
{
        NodePathCollection occls = find_all_matches( "**/occluder." + name );
        for ( int i = 0; i < occls.get_num_paths(); i++ )
        {
                g_render.clear_occluder( occls[i] );
        }
}

/**
* Disable all occluders present in the map.
*/
void Map::
disable_occluders()
{
        g_render.clear_occluder();
}

/**
* Enables all of the lights present in the map.
*/
void Map::
enable_lights()
{
        NodePathCollection lights = find_all_matches( "**/+Light" );
        for ( int i = 0; i < lights.get_num_paths(); i++ )
        {
                g_render.set_light( lights[i] );
        }
}

/**
* Enables the specified light.
*/
void Map::
enable_light( const string &name )
{
        NodePathCollection lights = find_all_matches( "**/" + name + "-*light;+i" );
        for ( int i = 0; i < lights.get_num_paths(); i++ )
        {
                g_render.set_light( lights[i] );
        }
}

/**
* Disables the specified light.
*/
void Map::
disable_light( const string &name )
{
        NodePathCollection lights = find_all_matches( "**/" + name + "-*light;+i" );
        for ( int i = 0; i < lights.get_num_paths(); i++ )
        {
                g_render.clear_light( lights[i] );
        }
}

/**
* Disables all of the lights present in the map.
*/
void Map::
disable_lights()
{
        NodePathCollection lights = find_all_matches( "**/+Light" );
        for ( int i = 0; i < lights.get_num_paths(); i++ )
        {
                g_render.clear_light( lights[i] );
        }
}

void Map::
enable_fog_controller()
{
        NodePathCollection fogs = find_all_matches( "**/+Fog" );
        for ( int i = 0; i < fogs.get_num_paths(); i++ )
        {
                g_render.set_fog( DCAST( Fog, fogs[i].node() ) );
        }
}

void Map::
disable_fog_controller()
{
        g_render.clear_fog();
}

/**
* Enables all of the doors present in the map.
* It will watch for the collision between the avatar and the door.
*/
void Map::
enable_doors()
{
        NodePathCollection doors = find_all_matches( "**/entity.func_door*" );
        for ( int i = 0; i < doors.get_num_paths(); i++ )
        {
                NodePath np = doors[i];
                Door *door = DCAST( Door, np.find( "**/door" ).node() );
                door->set_door_np( np.find( "**/door_origin" ) );
                _event_handler->add_hook( "enter" + door->get_name() + ".trigger", handle_door_touch, door );
                door->start_tick();
        }
}

void Map::
enable_door( const string &name )
{
        NodePathCollection doors = find_all_matches( "**/entity.func_door." + name );
        for ( int i = 0; i < doors.get_num_paths(); i++ )
        {
                NodePath np = doors[i];
                Door *door = DCAST( Door, np.find( "**/door" ).node() );
                door->set_door_np( np.find( "**/door_origin" ) );
                _event_handler->add_hook( "enter" + door->get_name() + ".trigger", handle_door_touch, door );
                door->start_tick();
        }
}

void Map::
disable_door( const string &name )
{
        NodePathCollection doors = find_all_matches( "**/entity.func_door." + name );
        for ( int i = 0; i < doors.get_num_paths(); i++ )
        {
                NodePath np = doors[i];
                Door *door = DCAST( Door, np.find( "**/door" ).node() );
                door->stop_tick();
                _event_handler->remove_hook( "enter" + door->get_name() + ".trigger", handle_door_touch, door );
        }
}

void Map::
disable_doors()
{
        NodePathCollection doors = find_all_matches( "**/entity.func_door*" );
        for ( int i = 0; i < doors.get_num_paths(); i++ )
        {
                NodePath np = doors[i];
                Door *door = DCAST( Door, np.find( "**/door" ).node() );
                door->stop_tick();
                _event_handler->remove_hook( "enter" + door->get_name() + ".trigger", handle_door_touch, door );
        }
}

void Map::
handle_door_touch( const Event *e, void *data )
{
        ( ( Door * )data )->handle_touch();
}

void Map::
enable_triggers()
{
        NodePathCollection trigs = find_all_matches( "**/entity.trigger_*" );
        for ( int i = 0; i < trigs.get_num_paths(); i++ )
        {
                NodePath trig = trigs[i];
                NodePath collNP = trig.find( "**/*.trigger" );
                _event_handler->add_hook( "enter" + collNP.get_name(), handle_trigger_touch );
        }
}

void Map::
disable_triggers()
{
        NodePathCollection trigs = find_all_matches( "**/entity.trigger_*" );
        for ( int i = 0; i < trigs.get_num_paths(); i++ )
        {
                NodePath trig = trigs[i];
                NodePath collNP = trig.find( "**/*.trigger" );
                _event_handler->remove_hook( "enter" + collNP.get_name(), handle_trigger_touch );
        }
}

void Map::
handle_trigger_touch( const Event *e )
{
}

void Map::
enable_skybox()
{
        NodePath sky_root = find( "**/skybox_root" );
        sky_root.reparent_to( g_base->_camera );
        sky_root.set_pos( 0, 0, 0 );
        sky_root.set_hpr( 0, 0, 0 );
}

NodePathCollection Map::
find_all_devshotcameras() const
{
        NodePathCollection npc = find_all_matches( "**/+DevshotCamera" );
        cout << "Map: found " << npc.get_num_paths() << " DevshotCameras" << endl;
        npc.ls();
        return npc;
}

void Map::
use_devshotcamera( const NodePath &cam )
{
        g_camera.reparent_to( g_render );
        g_camera.set_pos( cam.get_pos( g_render ) );
        g_camera.set_hpr( cam.get_hpr( g_render ) - LVector3( 90, 0, 0 ) );

        PN_stdfloat fov = DCAST( DevshotCamera, cam.node() )->get_fov();
        g_base->set_camera_fov( fov / ( 4. / 3. ) );
}

void Map::
use_random_devshotcamera()
{
        NodePathCollection npc = find_all_devshotcameras();
        use_devshotcamera( npc.get_path( _random.random_int( npc.get_num_paths() ) ) );
}