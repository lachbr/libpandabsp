#ifndef CONFIG_NETWORKSYSTEM_H
#define CONFIG_NETWORKSYSTEM_H

#include "dconfig.h"
#include "notifyCategoryProxy.h"

#ifdef BUILDING_NETWORKSYSTEM
#define EXPCL_NETWORKSYSTEM EXPORT_CLASS
#define EXPTP_NETWORKSYSTEM EXPORT_CLASS
#else
#define EXPCL_NETWORKSYSTEM IMPORT_CLASS
#define EXPTP_NETWORKSYSTEM IMPORT_CLASS
#endif

ConfigureDecl( config_networksystem, EXPCL_NETWORKSYSTEM, EXPTP_NETWORKSYSTEM )

NotifyCategoryDeclNoExport(networksystem)

extern EXPCL_NETWORKSYSTEM void init_libnetworksystem();

#endif // CONFIG_NETWORKSYSTEM_H