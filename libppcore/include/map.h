#ifndef __MAP_H__
#define __MAP_H__

#include "config_pandaplus.h"

#include <eventHandler.h>
#include <nodePath.h>
#include <event.h>
#include <nodePathCollection.h>
#include <randomizer.h>

class MapInfo;

class EXPCL_PANDAPLUS Map : public NodePath
{
public:

        Map();

        //void operator =(const Map &other);

        void enable_occluders();
        void enable_occluder( const string &name );
        void disable_occluder( const string &name );
        void disable_occluders();

        void enable_lights();
        void enable_light( const string &name );
        void disable_light( const string &name );
        void disable_lights();

        void enable_doors();
        void enable_door( const string &name );
        void disable_door( const string &name );
        void disable_doors();

        void enable_triggers();
        void disable_triggers();

        void enable_fog_controller();
        void disable_fog_controller();

        void enable_skybox();
        //void disable_skybox();

        void use_devshotcamera( const NodePath &cam );
        void use_random_devshotcamera();
        NodePathCollection find_all_devshotcameras() const;

        MapInfo *get_map_info() const;

private:
        static void handle_door_touch( const Event *e, void *data );
        static void handle_trigger_touch( const Event *e );

        EventHandler *_event_handler;
        Randomizer _random;
};

#endif // __MAP_H__
