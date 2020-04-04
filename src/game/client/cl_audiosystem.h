#pragma once

#include "config_clientdll.h"
#include "audiosystem.h"

class RenderSystem;

class EXPORT_CLIENT_DLL ClientAudioSystem : public AudioSystem
{
	DECLARE_CLASS( ClientAudioSystem, AudioSystem )

public:
	ClientAudioSystem();

	virtual const char *get_name() const;

	virtual bool initialize();
	virtual void update( double frametime );

private:
	RenderSystem *_rsys;
	LPoint3f _last_view_position;
};

extern ClientAudioSystem *claudio;