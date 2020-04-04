#include "emitsound.h"
#include "cl_audiosystem.h"

void emit_sound( const Filename &audiofile, const LPoint3 &position, int channel, float volume )
{
	AudioSound *sound = claudio->load_sfx( audiofile, true, channel );
	sound->set_volume( volume );
	sound->set_3d_attributes( position[0], position[1], position[2], 0, 0, 0 );
	sound->play();
}