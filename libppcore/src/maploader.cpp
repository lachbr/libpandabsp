#include "maploader.h"
#include "pp_globals.h"

#include <nodePathCollection.h>
#include <collisionNode.h>

Map MapLoader::_map;

/**
* Loads and sets up a map.
* Make sure to call unload_map to clean up an already existing map before loading a new one.
*/
void MapLoader::load_map( const string &file )
{

        // Load the map model.
        g_window->load_model( _map, file );

        // Enable all of the special objects, if they exist.
        _map.enable_occluders();
        _map.enable_lights();
        _map.enable_doors();
        _map.enable_triggers();
        _map.enable_fog_controller();
        _map.enable_skybox();

        // Show the map.
        _map.reparent_to( g_render );
}

/**
* Unloads the current loaded map.
*/
void MapLoader::unload_map()
{

        // Disable all of the special objects.
        _map.disable_occluders();
        _map.disable_lights();
        _map.disable_triggers();
        _map.disable_doors();

        // Remove the entire scene.
        _map.find( "**/scene" ).remove_node();

        // Hide the map.
        _map.reparent_to( g_hidden );
}

Map &MapLoader::get_map()
{
        return _map;
}