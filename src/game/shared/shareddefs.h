#pragma once

#ifdef CLIENT_DLL
#define CBaseComponent C_BaseComponent
#include "c_basecomponent.h"
#define DECLARE_COMPONENT(classname, basename) DECLARE_CLIENTCLASS(classname, basename)
#else
#include "basecomponent.h"
#define DECLARE_COMPONENT(classname, basename) DECLARE_SERVERCLASS(classname, basename)
#endif


