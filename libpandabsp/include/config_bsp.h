#ifndef CONFIG_BSP_H
#define CONFIG_BSP_H

#include <dconfig.h>

#ifdef BUILDING_LIBPANDABSP
#define EXPCL_PANDABSP __declspec(dllexport)
#define EXPTP_PANDABSP __declspec(dllexport)
#else
#define EXPCL_PANDABSP __declspec(dllimport)
#define EXPTP_PANDABSP __declspec(dllimport)
#endif

ConfigureDecl( config_bsp, EXPCL_PANDABSP, EXPTP_PANDABSP );

extern EXPCL_PANDABSP void init_libpandabsp();

#endif // CONFIG_BSP_H