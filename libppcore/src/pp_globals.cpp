#include "pp_globals.h"

#define EMPTY_NP NodePath()

// Give the global variables default values. They should be set later on by code that links with PandaPlus.
WindowFramework *g_window = ( WindowFramework * )NULL;
PandaFramework *g_framework = ( PandaFramework * )NULL;
PT( AudioManager ) g_music_mgr = ( PT( AudioManager ) )NULL;
PT( AudioManager ) g_sfx_mgr = ( PT( AudioManager ) )NULL;
ClientRepository *g_cr = ( ClientRepository * )NULL;
ClientRepository *g_air = ( ClientRepository * )NULL;
Audio3DManager *g_audio3d = ( Audio3DManager * )NULL;
PPBase *g_base = ( PPBase * )NULL;
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