#include "baseanimating.h"

NotifyCategoryDef( baseanimating, "" )

CBaseAnimating::CBaseAnimating() :
	CBaseEntity(),
	CBaseAnimatingShared()
{
}

void CBaseAnimating::set_model( const std::string &model_path )
{
	CBaseAnimatingShared::set_model( model_path );
	_model_path = model_path;
	_model_np.reparent_to( _np );
}

void CBaseAnimating::set_model_origin( const LPoint3 &origin )
{
	CBaseAnimatingShared::set_model_origin( origin );
	_model_origin = origin;
}

void CBaseAnimating::set_model_angles( const LVector3 &angles )
{
	CBaseAnimatingShared::set_model_angles( angles );
	_model_angles = angles;
}

void CBaseAnimating::set_model_scale( const LVector3 &scale )
{
	CBaseAnimatingShared::set_model_scale( scale );
	_model_scale = scale;
}

void CBaseAnimating::set_animation( const std::string &animation )
{
	CBaseAnimatingShared::set_animation( animation );
	_animation = animation;
}

void CBaseAnimating::handle_anim_event( AnimEvent_t *ae )
{
	baseanimating_cat.debug()
		<< "Anim event " << ae->event_name << " at frame " << ae->event_frame << "\n";
}

void CBaseAnimating::animevents_func( CEntityThinkFunc *func, void *data )
{
	CBaseAnimating *self = (CBaseAnimating *)data;

	if ( self->_playing_anim && self->_hbundle )
	{
		self->update_blending();

		// Advance the animation frame and joint positions if we have to.
		self->_hbundle->get_bundle()->update();

		int curr_frame = self->_playing_anim->anim_control->get_frame();
		if ( curr_frame != self->_playing_anim->last_frame )
		{
			// Check if there is an anim event associated with this frame.
			for ( AnimEvent_t ae : self->_playing_anim->anim_events )
			{
				if ( ae.event_frame == curr_frame )
				{
					self->handle_anim_event( &ae );
				}
			}
			self->_playing_anim->last_frame = curr_frame;
		}
	}
}

void CBaseAnimating::spawn()
{
	BaseClass::spawn();

	make_think_func( "animevents", animevents_func, this );
}

void CBaseAnimating::init( entid_t entum )
{
	BaseClass::init( entum );
	_model_origin = LPoint3( 0 );
	_model_angles = LVector3( 0 );
	_model_scale = LVector3( 1 );
}

IMPLEMENT_SERVERCLASS_ST( CBaseAnimating )
	SendPropString( PROPINFO( _model_path ) ),
	SendPropVec3( PROPINFO( _model_origin ) ),
	SendPropVec3( PROPINFO( _model_angles ) ),
	SendPropVec3( PROPINFO( _model_scale ) ),
	SendPropString( PROPINFO( _animation ) )
END_SEND_TABLE()