#pragma once

#include "pandabase.h"

class C_BasePlayer;

class LocalPlayer
{
public:
	LocalPlayer();

	void set_player( C_BasePlayer *player );
	C_BasePlayer *get_player() const;

protected:
	C_BasePlayer *_player;
};

INLINE void LocalPlayer::set_player( C_BasePlayer *player )
{
	_player = player;
}

INLINE C_BasePlayer *LocalPlayer::get_player() const
{
	return _player;
}