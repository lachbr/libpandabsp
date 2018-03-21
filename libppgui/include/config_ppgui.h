#ifndef CONFIG_PPGUI_H
#define CONFIG_PPGUI_H

#include <dconfig.h>

#ifdef BUILDING_PPGUI
#define EXPCL_PPGUI __declspec(dllexport)
#define EXPTP_PPGUI __declspec(dllexport)
#else
#define EXPCL_PPGUI __declspec(dllimport)
#define EXPTP_PPGUI __declspec(dllimport)
#endif // BUILDING_PPGUI

ConfigureDecl( config_ppGui, EXPCL_PPGUI, EXPTP_PPGUI );

extern EXPCL_PPGUI void init_libppgui();

#endif // CONFIG_PPGUI_H