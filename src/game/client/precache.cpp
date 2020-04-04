#include "precache.h"
#include "audiosystem.h"
#include "clientbase.h"

void precache_sound( const Filename &audiofile, bool positional, int mgr )
{
	AudioSystem *audio;
	cl->get_game_system_of_type( audio );

	AudioSound *sound = audio->load_sfx( audiofile, positional, mgr );
	if ( sound )
	{
		// Play and stop to get audio data in memory.
		sound->play();
		sound->stop();
	}
}