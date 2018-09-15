#ifndef CONFIG_RECASTNAVIGATION_H
#define CONFIG_RECASTNAVIGATION_H

#pragma once

#include "pandabase.h"
#include "notifyCategoryProxy.h"
#include "configVariableDouble.h"
#include "configVariableString.h"
#include "configVariableInt.h"

using namespace std;

#ifndef CPPPARSER
#ifdef BUILDING_PANDARN
#define EXPCL_PANDARN __declspec(dllexport)
#define EXPTP_PANDARN 
#else
#define EXPCL_PANDARN __declspec(dllimport)
#define EXPTP_PANDARN 
#endif
#else
#define EXPCL_PANDARN
#define EXPTP_PANDARN
#endif

NotifyCategoryDecl(recastnavigation, EXPCL_PANDARN, EXPTP_PANDARN);

extern void init_librecastnavigation();

#endif
