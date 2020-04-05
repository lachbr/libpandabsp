#pragma once

#include "c_baseentity.h"
#include "baseanimating_shared.h"

class EXPORT_CLIENT_DLL C_BaseAnimating : public C_BaseEntity, public CBaseAnimatingShared
{
	DECLARE_CLIENTCLASS( C_BaseAnimating, C_BaseEntity )

public:
	C_BaseAnimating();

	virtual void spawn();

private:
	static void anim_blending_func( CEntityThinkFunc *func, void *data );

public:
	std::string _model_path;
	LPoint3 _model_origin;
	LVector3 _model_angles;
	LVector3 _model_scale;
	std::string _animation;
};