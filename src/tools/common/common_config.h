#ifndef BSPCOMMON_CONFIG_H_
#define BSPCOMMON_CONFIG_H_

#ifdef BUILDING_BSPCOMMON
#define _BSPEXPORT __declspec(dllexport)
#else
#define _BSPEXPORT __declspec(dllimport)
#endif

#endif // BSPCOMMON_CONFIG_H_