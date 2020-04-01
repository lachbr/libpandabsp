#pragma once

#include "baseentity.h"
#include "baseanimating_shared.h"

NotifyCategoryDeclNoExport( baseanimating )

class EXPORT_SERVER_DLL CBaseAnimating : public CBaseEntity, public CBaseAnimatingShared
{
	DECLARE_CLASS( CBaseAnimating, CBaseEntity )
	DECLARE_SERVERCLASS()

public:
	CBaseAnimating();

	virtual void init( entid_t entnum );

	virtual void spawn();

	virtual void set_model( const std::string &model_path );
	virtual void set_model_origin( const LPoint3 &origin );
	virtual void set_model_angles( const LVector3 &angles );
	virtual void set_model_scale( const LVector3 &scale );

	virtual void set_animation( const std::string &anim );

	virtual void handle_anim_event( AnimEvent_t *ae );

private:
	static void animevents_func( CEntityThinkFunc *func, void *data );

public:
	NetworkString( _model_path );
	NetworkVec3( _model_origin );
	NetworkVec3( _model_angles );
	NetworkVec3( _model_scale );
	NetworkString( _animation );
};