#pragma once

#include "config_game_shared.h"

#include "pvector.h"

#ifdef BUILDING_SERVER_DLL
#define EXPORT_SERVER_DLL __declspec(dllexport)
#else
#define EXPORT_SERVER_DLL __declspec(dllimport)
#endif

extern EXPORT_SERVER_DLL ConfigVariableInt sv_maxclients;
extern EXPORT_SERVER_DLL ConfigVariableInt sv_tickrate;
extern EXPORT_SERVER_DLL ConfigVariableInt sv_clockcorrection_msecs;

typedef pvector<uint32_t> vector_uint32;