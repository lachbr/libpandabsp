#pragma once

#include "config_game_shared.h"

#include <nodePath.h>
#include <aa_luse.h>
#include <animControl.h>
#include <pmap.h>
#include <partBundleHandle.h>

class AnimEvent_t
{
public:
	int event_frame;
	std::string event_name;
};

class AnimInfo_t
{
public:
	pvector<AnimEvent_t> anim_events;
	std::string name;
	PT( AnimControl ) anim_control;
	bool loop;
	float play_rate;
	int start_frame;
	int end_frame;
	bool snap;
	float fade_in;
	float fade_out;

	double play_time;

	int last_frame;
};

class EXPORT_GAME_SHARED CBaseAnimatingShared
{
public:
	CBaseAnimatingShared();

	virtual void set_model( const std::string &model_path );
	virtual void set_model_origin( const LPoint3 &origin );
	virtual void set_model_angles( const LVector3 &angles );
	virtual void set_model_scale( const LVector3 &scale );

	virtual void set_animation( const std::string &animation );

	void update_blending();
	void reset_blends();

public:
	NodePath _model_np;

	typedef pmap<std::string, AnimInfo_t> AnimMap;
	AnimMap _anims;

	AnimInfo_t *_last_anim;
	AnimInfo_t *_playing_anim;

	bool _doing_blend;

	PT( PartBundleHandle ) _hbundle;
};