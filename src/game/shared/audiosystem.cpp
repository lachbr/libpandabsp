#include "audiosystem.h"

#include "notifyCategoryProxy.h"

NotifyCategoryDeclNoExport( audiosystem )
NotifyCategoryDef( audiosystem, "" )

IMPLEMENT_CLASS( AudioSystem )

AudioSystem::AudioSystem()
{
	_music_manager = nullptr;
}

const char *AudioSystem::get_name() const
{
	return "AudioSystem";
}

AudioSound *AudioSystem::load_music( const Filename &musicfile )
{
	PT( AudioSound ) song = _music_manager->get_sound( musicfile );
	if ( song )
	{
		_music_cache[musicfile.get_basename_wo_extension()] = song;
	}
	return song;
}

void AudioSystem::play_music( const std::string &songname, bool loop, float volume )
{
	stop_music();

	int isong = _music_cache.find( songname );
	if ( isong != -1 )
	{
		AudioSound *song = _music_cache.get_data( isong );
		song->set_volume( volume );
		song->set_loop( loop );
		song->play();
		_current_song = song;
	}
	else
	{
		audiosystem_cat.warning()
			<< "Song `" << songname << "` not found\n";
	}
}

void AudioSystem::stop_music()
{
	if ( _current_song )
	{
		_current_song->stop();
		_current_song = nullptr;
	}
}

bool AudioSystem::initialize()
{
	_music_manager = AudioManager::create_AudioManager();
	
	create_sfx_mgr();

	return true;
}

void AudioSystem::shutdown()
{
	_music_manager = nullptr;
	_sfx_managers.clear();
}

void AudioSystem::update( double frametime )
{
	_music_manager->update();

	size_t count = _sfx_managers.size();
	for ( size_t i = 0; i < count; i++ )
	{
		_sfx_managers[i]->update();
	}
}

int AudioSystem::create_sfx_mgr()
{
	int imgr = _sfx_managers.size();

	PT( AudioManager ) mgr = AudioManager::create_AudioManager();
	// Use feet for audio 3d
	mgr->audio_3d_set_distance_factor( 3.28f );
	_sfx_managers.push_back( mgr );

	return imgr;
}

AudioSound *AudioSystem::load_sfx( const Filename &audiofile, bool positional, int mgr )
{
	SFXEntry lookup;
	lookup.filename = audiofile;
	lookup.positional = positional;
	lookup.mgr = mgr;

	auto itr = std::find_if( _audio_cache.begin(), _audio_cache.end(), [audiofile, positional, mgr]( const SFXEntry &other )
	{
		return audiofile == other.filename && positional == other.positional && mgr == other.mgr;
	} );

	if ( itr != _audio_cache.end() )
	{
		return itr->sound;
	}

	PT( AudioSound ) sound = _sfx_managers[mgr]->get_sound( audiofile, positional );
	if ( sound )
	{
		lookup.sound = sound;
		_audio_cache.push_back( lookup );
	}
	
	return sound;
}

/**
 * Releases the AudioSystem's references on all past loaded sounds.
 * This should unload all sound data... unless you are holding a
 * reference to one of the sounds somewhere in your code.
 */
void AudioSystem::release_all_sounds()
{
	_audio_cache.clear();
}

/**
 * Releases the AudioSystem's references on all past loaded songs.
 * This should unload all song data... unless you are holding a
 * reference to one of the songs somewhere in your code.
 */
void AudioSystem::release_all_music()
{
	stop_music();
	_music_cache.clear();
}