#include "pp_globals.h"

#define EMPTY_NP NodePath()

// Give the global variables default values. They should be set later on by code that links with PandaPlus.
WindowFramework *g_window = nullptr;
PandaFramework *g_framework = nullptr;
PT( AudioManager ) g_music_mgr = nullptr;
PT( AudioManager ) g_sfx_mgr = nullptr;
ClientRepository *g_cr = nullptr;
ClientRepository *g_air = nullptr;
Audio3DManager *g_audio3d = nullptr;
PPBase *g_base = nullptr;
NodePath g_render = EMPTY_NP;
NodePath g_hidden = EMPTY_NP;
NodePath g_render2d = EMPTY_NP;
NodePath g_aspect2d = EMPTY_NP;
NodePath g_camera = EMPTY_NP;

// These ones doesn't need to be set by anything else.
PT( AsyncTaskManager ) g_task_mgr = AsyncTaskManager::get_global_ptr();
PT( ClockObject ) g_global_clock = ClockObject::get_global_clock();
InputState g_input_state = InputState();
BitMask32 g_wall_bitmask = BitMask32( 1 );
BitMask32 g_floor_bitmask = BitMask32( 2 );
BitMask32 g_event_bitmask = BitMask32( 3 );