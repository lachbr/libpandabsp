#pragma once

#include "config_game_shared.h"
#include "shareddefs.h"

#ifdef CLIENT_DLL
#define SceneComponent C_SceneComponent
#define BaseAnimating C_BaseAnimating
#else
#define SceneComponent CSceneComponent
#define BaseAnimating CBaseAnimating
#endif

class SceneComponent;

#include <nodePath.h>
#include <luse.h>
#include <animControl.h>
#include <pmap.h>
#include <partBundleHandle.h>

#include "actor.h"

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

class EXPORT_GAME_SHARED BaseAnimating : public CBaseComponent
{
	DECLARE_COMPONENT( BaseAnimating, CBaseComponent )

public:
	BaseAnimating();

#if !defined( CLIENT_DLL )
	virtual void handle_anim_event( AnimEvent_t *ae );
#endif

	virtual void reset();

	virtual bool initialize();

	virtual void set_model( const std::string &model_path );
	virtual void set_model_origin( const LPoint3 &origin );
	virtual void set_model_angles( const LVector3 &angles );
	virtual void set_model_scale( const LVector3 &scale );

	virtual void set_animation( const std::string &animation );

	virtual void update( double frametime );

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

	SceneComponent *_scene;

	pvector<PT( CActor )> _actors;

#if !defined( CLIENT_DLL )
	NetworkString( model_path );
	NetworkString( animation );
	NetworkVec3( model_origin );
	NetworkVec3( model_angles );
	NetworkVec3( model_scale );
#else
	std::string model_path;
	std::string animation;
	LVector3f model_origin;
	LVector3f model_angles;
	LVector3f model_scale;
#endif
};