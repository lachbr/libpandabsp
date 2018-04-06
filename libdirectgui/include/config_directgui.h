#ifndef CONFIG_DIRECTGUI_H
#define CONFIG_DIRECTGUI_H

#include <dconfig.h>

#ifdef BUILDING_LIBDIRECTGUI
#define EXPCL_DIRECTGUI __declspec(dllexport)
#define EXPTP_DIRECTGUI __declspec(dllexport)
#else // BUILDING_LIBDIRECTGUI
#define EXPCL_DIRECTGUI __declspec(dllimport)
#define EXPTP_DIRECTGUI __declspec(dllimport)
#endif // BUILDING_LIBDIRECTGUI

ConfigureDecl( libdirectgui, EXPCL_DIRECTGUI, EXPTP_DIRECTGUI );

extern EXPCL_DIRECTGUI void init_libdirectgui();

#endif // CONFIG_DIRECTGUI_H