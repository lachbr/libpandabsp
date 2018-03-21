#ifndef __CONFIG_VIFPARSER_H__
#define __CONFIG_VIFPARSER_H__

#ifdef BUILDING_VIFPARSER
#define EXPCL_VIF __declspec(dllexport)
#define EXPTP_VIF __declspec(dllexport)
#else
#define EXPCL_VIF __declspec(dllimport)
#define EXPTP_VIF __declspec(dllimport)
#endif // BUILDING_VIFPARSER

#endif // __CONFIG_VIFPARSER_H__