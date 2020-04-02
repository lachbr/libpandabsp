#pragma once

#include "config_game_shared.h"
#include "igamesystem.h"

#include "audio_3d_manager.h"

#include <audioManager.h>

class HostBase;

class EXPORT_GAME_SHARED AudioSystem : public IGameSystem
{
	DECLARE_CLASS( AudioSystem, IGameSystem )
public:
	AudioSystem();

	virtual const char *get_name() const;

	virtual bool initialize();
	virtual void shutdown();
	virtual void update( double frametime );

	AudioManager *get_music_mgr() const;
	AudioManager *get_sfx_mgr( int n = 0 ) const;
	Audio3DManager *get_audio3d( int n = 0 ) const;

	PT( AudioSound ) load_sfx( int mgr, const Filename &audiofile, bool is3d = false );
	PT( AudioSound ) load_sfx( int mgr, const Filename &audiofile, const NodePath &attachto );

	void attach_sound( int mgr, AudioSound *snd, const NodePath &attachto );
	void detach_sound( int mgr, AudioSound *snd );

	int create_sfx_mgr();
	// Creates an audio manager with 3D positioning enabled
	int create_sfx_mgr( const NodePath &listener_target, const NodePath &root );

private:
	class SFXManager
	{
	public:
		PT( AudioManager ) sfxmgr;
		PT( Audio3DManager ) audio3d;
	};

	PT( AudioManager ) _music_manager;
	pvector<SFXManager> _sfx_managers;
};

INLINE const char *AudioSystem::get_name() const
{
	return "AudioSystem";
}

INLINE AudioManager *AudioSystem::get_music_mgr() const
{
	return _music_manager;
}

INLINE AudioManager *AudioSystem::get_sfx_mgr( int n ) const
{
	return _sfx_managers[n].sfxmgr;
}

INLINE Audio3DManager *AudioSystem::get_audio3d( int n ) const
{
	return _sfx_managers[n].audio3d;
}