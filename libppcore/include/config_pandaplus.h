#ifndef __CONFIG_PANDAPLUS_H__
#define __CONFIG_PANDAPLUS_H__

#include <pandabase.h>
#include <dconfig.h>
#include <notifyCategoryProxy.h>

#ifdef BUILDING_PANDAPLUS
#define EXPCL_PANDAPLUS __declspec(dllexport)
#define EXPTP_PANDAPLUS __declspec(dllexport)
#else
#define EXPCL_PANDAPLUS __declspec(dllimport)
#define EXPTP_PANDAPLUS __declspec(dllimport)
#endif

ConfigureDecl( config_pandaPlus, EXPCL_PANDAPLUS, EXPTP_PANDAPLUS );

extern ConfigVariableBool gw_debug_indicator;
extern ConfigVariableBool gw_floor_sphere;
extern ConfigVariableBool gw_early_event_sphere;
extern ConfigVariableBool gw_fluid_pusher;

extern ConfigVariableDouble rp_shadow_update_distance;
extern ConfigVariableInt rp_shadow_max_updates;
extern ConfigVariableInt rp_atlas_size;
extern ConfigVariableInt rp_lighting_culling_grid_size_x;
extern ConfigVariableInt rp_lighting_culling_grid_size_y;
extern ConfigVariableInt rp_lighting_culling_grid_slices;
extern ConfigVariableInt rp_lighting_culling_slice_width;
extern ConfigVariableInt rp_lighting_max_lights_per_cell;

extern EXPCL_PANDAPLUS void init_libpandaplus();

#endif // __CONFIG_PANDAPLUS_H__