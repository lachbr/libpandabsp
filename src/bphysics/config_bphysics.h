#include "dconfig.h"

#ifdef BUILDING_BPHYSICS
#define EXPORT_BPHYSICS EXPORT_CLASS
#else
#define EXPORT_BPHYSICS IMPORT_CLASS
#endif

extern EXPORT_BPHYSICS void init_bphysics();
