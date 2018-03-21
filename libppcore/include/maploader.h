#ifndef __MAP_LOADER_H__
#define __MAP_LOADER_H__

#include "config_pandaplus.h"
#include "map.h"

/**
* This class is used to load and setup .bam maps converted from Valve's Hammer World Editor (vmf2bam).
*/
class EXPCL_PANDAPLUS MapLoader
{
public:
        static void load_map( const string &file );
        static void unload_map();

        // Get the instance of the current loaded map.
        static Map &get_map();

private:
        static Map _map;
};

#endif // __MAP_LOADER_H__