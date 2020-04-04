#pragma once

#include "filename.h"
#include "luse.h"

// Emit a sound
extern void emit_sound( const Filename &audiofile, const LPoint3 &position,
			int channel = 0, float volume = 1.0f );