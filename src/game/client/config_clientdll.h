#pragma once

#include "config_game_shared.h"

#ifdef BUILDING_CLIENT_DLL
#define EXPORT_CLIENT_DLL __declspec(dllexport)
#else
#define EXPORT_CLIENT_DLL __declspec(dllimport)
#endif // BUILDING_CLIENT_DLL

extern EXPORT_CLIENT_DLL ConfigVariableDouble cl_normspeed;
extern EXPORT_CLIENT_DLL ConfigVariableDouble cl_walkspeed;
extern EXPORT_CLIENT_DLL ConfigVariableDouble cl_runspeed;
extern EXPORT_CLIENT_DLL ConfigVariableDouble cl_mousesensitivity;
extern EXPORT_CLIENT_DLL ConfigVariableDouble cl_duckspeed;
extern EXPORT_CLIENT_DLL ConfigVariableInt cl_cmdrate;
extern EXPORT_CLIENT_DLL ConfigVariableInt cl_updaterate;
extern EXPORT_CLIENT_DLL ConfigVariableDouble cl_interp;

extern EXPORT_CLIENT_DLL float get_client_interp_amount();