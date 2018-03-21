#ifndef __PP_GLOBALS_H__
#define __PP_GLOBALS_H__

#include <pandaFramework.h>
#include <audioManager.h>
#include <bitMask.h>

#include "config_pandaplus.h"
#include "inputstate.h"
#include "ppbase.h"

#define PID_NONE 0
#define ZID_NONE 0
#define TaskArgs GenericAsyncTask *task, void *data
#define EventArgs const Event *ev, void *data

extern EXPCL_PANDAPLUS WindowFramework *g_window;
extern EXPCL_PANDAPLUS PandaFramework *g_framework;
extern EXPCL_PANDAPLUS PT( AudioManager ) g_music_mgr;
extern EXPCL_PANDAPLUS PT( AudioManager ) g_sfx_mgr;
extern EXPCL_PANDAPLUS PT( AsyncTaskManager ) g_task_mgr;
extern EXPCL_PANDAPLUS NodePath g_render;
extern EXPCL_PANDAPLUS NodePath g_hidden;
extern EXPCL_PANDAPLUS NodePath g_render2d;
extern EXPCL_PANDAPLUS NodePath g_aspect2d;
extern EXPCL_PANDAPLUS NodePath g_camera;
extern EXPCL_PANDAPLUS InputState g_input_state;
extern EXPCL_PANDAPLUS PT( ClockObject ) g_global_clock;
extern EXPCL_PANDAPLUS PPBase *g_base;
extern EXPCL_PANDAPLUS BitMask32 g_wall_bitmask;
extern EXPCL_PANDAPLUS BitMask32 g_floor_bitmask;
extern EXPCL_PANDAPLUS BitMask32 g_event_bitmask;

class Audio3DManager;
extern EXPCL_PANDAPLUS Audio3DManager *g_audio3d;

// ClientRepository is implemented in libppdistributed.
class ClientRepository;
extern EXPCL_PANDAPLUS ClientRepository *g_cr;
extern EXPCL_PANDAPLUS ClientRepository *g_air;

#endif // __PP_GLOBALS_H__