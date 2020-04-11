#pragma once

#include "config_game_shared.h"
#include "shareddefs.h"

#include "nodePath.h"
#include "luse.h"

#ifdef CLIENT_DLL
#define SceneComponent C_SceneComponent
#else
#define SceneComponent CSceneComponent
#endif

class SceneComponent : public CBaseComponent
{
	DECLARE_COMPONENT( SceneComponent, CBaseComponent )

public:
	SceneComponent();

	virtual bool initialize();
	virtual void shutdown();
	virtual void spawn();

	void update_parent_entity( int parent );

	void set_origin( const LPoint3f &origin );
	void set_angles( const LVector3f &angles );
	void set_scale( const LVector3f &scale );

#if !defined( CLIENT_DLL )
	virtual void init_mapent();
	void transition_xform( const NodePath &landmark_np, const LMatrix4 &transform );
#endif

#ifdef CLIENT_DLL
	virtual void post_interpolate();
#endif

public:
	NodePath np;

#ifdef CLIENT_DLL
	int parent_entity;
	LVector3f origin;
	LVector3f angles;
	LVector3f scale;
	CInterpolatedVar<LVector3f> iv_origin;
	CInterpolatedVar<LVector3f> iv_angles;
	CInterpolatedVar<LVector3f> iv_scale;
#else
	NetworkVar( int, parent_entity );
	NetworkVec3( origin );
	NetworkVec3( angles );
	NetworkVec3( scale );
	LMatrix4f landmark_relative_transform;
#endif
};

inline void SceneComponent::set_origin( const LPoint3f &org )
{
	origin = org;
	np.set_pos( origin );
}

inline void SceneComponent::set_angles( const LVector3f &ang )
{
	angles = ang;
	np.set_hpr( angles );
}

inline void SceneComponent::set_scale( const LVector3f &sc )
{
	scale = sc;
	np.set_scale( sc );
}
