#pragma once

#include "config_game_shared.h"
#include <bitMask.h>

extern EXPORT_GAME_SHARED const BitMask32 wall_mask;
extern EXPORT_GAME_SHARED const BitMask32 floor_mask;
extern EXPORT_GAME_SHARED const BitMask32 event_mask;
extern EXPORT_GAME_SHARED const BitMask32 useable_mask;
extern EXPORT_GAME_SHARED const BitMask32 camera_mask;
extern EXPORT_GAME_SHARED const BitMask32 character_mask;

#define WORLD_MASK wall_mask|floor_mask