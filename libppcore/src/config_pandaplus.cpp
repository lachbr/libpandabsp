#include "config_pandaplus.h"

#include "door.h"
#include "mapinfo.h"
#include "textureanimation.h"
#include "sun.h"
#include "devshotcamera.h"

ConfigureDef( config_pandaPlus );
ConfigureFn( config_pandaPlus )
{
        init_libpandaplus();
}

// GRAVITY WALKER:

ConfigVariableBool gw_debug_indicator
( "want-avatar-physics-indicator", false );

ConfigVariableBool gw_floor_sphere
( "want-floor-sphere", false );

ConfigVariableBool gw_early_event_sphere
( "early-event-sphere", true );

ConfigVariableBool gw_fluid_pusher
( "want-fluid-pusher", true );

// RENDER PIPELINE:
// The default values need to be corrected later:

ConfigVariableDouble rp_shadow_update_distance
( "rp-shadow-update-distance", 500.0 );

ConfigVariableInt rp_shadow_max_updates
( "rp-shadow-max-updates", 5 );

ConfigVariableInt rp_atlas_size
( "rp-atlas-size", 5 );

ConfigVariableInt rp_lighting_culling_grid_size_x
( "rp-lighting-culling-grid-size-x", 1 );

ConfigVariableInt rp_lighting_culling_grid_size_y
( "rp-lighting-culling-grid-size-y", 1 );

ConfigVariableInt rp_lighting_culling_grid_slices
( "rp-lighting-culling-grid-slices", 1 );

ConfigVariableInt rp_lighting_culling_slice_width
( "rp-lighting-culling-size-width", 1 );

ConfigVariableInt rp_lighting_max_lights_per_cell
( "rp-lighting-max-lights-per-cell", 1 );

void init_libpandaplus()
{
        static bool initialized = false;
        if ( initialized )
        {
                return;
        }
        initialized = true;

        TextureAnimation::init_type();
        Door::init_type();
        MapInfo::init_type();
        Sun::init_type();
        DevshotCamera::init_type();

        Door::register_with_read_factory();
        MapInfo::register_with_read_factory();
        Sun::register_with_read_factory();
        DevshotCamera::register_with_read_factory();
}