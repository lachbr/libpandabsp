/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file config_bphysics.h
 * @author Brian Lach
 * @date April 27, 2020
 *
 * @desc Configuration header for Brian's Physics Library for Panda3D.
 */

#include "dconfig.h"

#ifdef BUILDING_BPHYSICS
#define EXPORT_BPHYSICS EXPORT_CLASS
#else
#define EXPORT_BPHYSICS IMPORT_CLASS
#endif

extern EXPORT_BPHYSICS void init_bphysics();
