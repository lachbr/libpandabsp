#include "audiosystem.h"

IMPLEMENT_CLASS( AudioSystem )

AudioSystem::AudioSystem()
{
	_music_manager = nullptr;
}

bool AudioSystem::initialize()
{
	_music_manager = AudioManager::create_AudioManager();
	
	// Developer must manually call create_sfx_mgr()

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
		SFXManager &mgr = _sfx_managers[i];
		if ( mgr.audio3d )
		{
			mgr.audio3d->update();
		}
		if ( mgr.sfxmgr )
		{
			mgr.sfxmgr->update();
		}
	}
}

int AudioSystem::create_sfx_mgr()
{
	int imgr = _sfx_managers.size();

	PT( AudioManager ) mgr = AudioManager::create_AudioManager();
	SFXManager sfxmgr;
	sfxmgr.sfxmgr = mgr;
	sfxmgr.audio3d = nullptr;
	_sfx_managers.push_back( sfxmgr );

	return imgr;
}

int AudioSystem::create_sfx_mgr( const NodePath &listener_target, const NodePath &root )
{
	int imgr = _sfx_managers.size();

	PT( AudioManager ) mgr = AudioManager::create_AudioManager();
	PT( Audio3DManager ) audio3d = new Audio3DManager( mgr, listener_target, root );
	audio3d->set_drop_off_factor( 0.15f );
	audio3d->set_doppler_factor( 0.15f );
	SFXManager sfxmgr;
	sfxmgr.sfxmgr = mgr;
	sfxmgr.audio3d = audio3d;
	_sfx_managers.push_back( sfxmgr );

	return imgr;
}

PT( AudioSound ) AudioSystem::load_sfx( int mgr, const Filename &audiofile, bool is3d )
{
	return _sfx_managers[mgr].sfxmgr->get_sound( audiofile, is3d );
}

PT( AudioSound ) AudioSystem::load_sfx( int mgr, const Filename &audiofile, const NodePath &attachto )
{
	SFXManager &sfxmgr = _sfx_managers[mgr];
	PT( AudioSound ) snd = sfxmgr.sfxmgr->get_sound( audiofile, true );
	sfxmgr.audio3d->attach_sound_to_object( snd, attachto );
	return snd;
}

void AudioSystem::attach_sound( int mgr, AudioSound *snd, const NodePath &attachto )
{
	_sfx_managers[mgr].audio3d->attach_sound_to_object( snd, attachto );
}

void AudioSystem::detach_sound( int mgr, AudioSound *snd )
{
	_sfx_managers[mgr].audio3d->detach_sound( snd );
}