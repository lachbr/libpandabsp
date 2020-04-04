#pragma once

#include "config_game_shared.h"
#include "igamesystem.h"

#include "audio_3d_manager.h"

#include <audioManager.h>
#include <pvector.h>
#include <simpleHashMap.h>

class EXPORT_GAME_SHARED AudioSystem : public IGameSystem
{
	DECLARE_CLASS( AudioSystem, IGameSystem )
public:
	AudioSystem();

	virtual const char *get_name() const;

	virtual bool initialize();
	virtual void shutdown();
	virtual void update( double frametime );

	AudioManager *get_sfx_mgr( int n = 0 ) const;

	AudioSound *load_music( const Filename &musicfile );
	AudioManager *get_music_mgr() const;
	void play_music( const std::string &songname, bool loop = true, float volume = 1.0f );
	void stop_music();

	AudioSound *load_sfx( const Filename &audiofile, bool positional = false, int mgr = 0 );

	int create_sfx_mgr();

	void release_all_sounds();
	void release_all_music();

protected:
	class SFXEntry
	{
	public:
		int mgr;
		bool positional;
		Filename filename;
		PT( AudioSound ) sound;

		SFXEntry()
		{
		}

		SFXEntry( const SFXEntry &other )
		{
			mgr = other.mgr;
			positional = other.positional;
			filename = other.filename;
			sound = other.sound;
		}
	};

	PT( AudioManager ) _music_manager;
	pvector<PT( AudioManager )> _sfx_managers;

	AudioSound *_current_song;

	// Maintains a reference count on all loaded sounds so the sound data
	// doesn't get deleted until we want it to.
	SimpleHashMap<std::string, PT( AudioSound ), string_hash> _music_cache;
	pvector<SFXEntry> _audio_cache;
};

INLINE AudioManager *AudioSystem::get_music_mgr() const
{
	return _music_manager;
}

INLINE AudioManager *AudioSystem::get_sfx_mgr( int n ) const
{
	return _sfx_managers[n];
}