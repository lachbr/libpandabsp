/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file precache.h
 * @author Brian Lach
 * @date April 03, 2020
 *
 * @desc Preloading/precaching helpers
 */

#pragma once

#include "config_clientdll.h"

#include "filename.h"

extern EXPORT_CLIENT_DLL void precache_sound( const Filename &audiofile, bool positional = false, int mgr = 0 );
extern EXPORT_CLIENT_DLL void precache_material( const Filename &matfile );