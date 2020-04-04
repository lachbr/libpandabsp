#include "c_baseanimating.h"

C_BaseAnimating::C_BaseAnimating() :
	C_BaseEntity(),
	CBaseAnimatingShared()
{
}

void RecvProxy_ModelPath( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	RecvProxy_String( prop, object, out, dgi );

	C_BaseAnimating *self = (C_BaseAnimating *)object;
	self->set_model( self->_model_path );
	self->_model_np.reparent_to( self->get_node_path() );
}

void RecvProxy_ModelOrigin( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	RecvProxy_Vec<LVector3>( prop, object, out, dgi );

	C_BaseAnimating *self = (C_BaseAnimating *)object;
	self->set_model_origin( self->_model_origin );
	std::cout << self->_model_origin << std::endl;
}

void RecvProxy_ModelAngles( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	RecvProxy_Vec<LVector3>( prop, object, out, dgi );

	C_BaseAnimating *self = (C_BaseAnimating *)object;
	self->set_model_angles( self->_model_angles );
}

void RecvProxy_ModelScale( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	RecvProxy_Vec<LVector3>( prop, object, out, dgi );

	C_BaseAnimating *self = (C_BaseAnimating *)object;
	self->set_model_scale( self->_model_scale );
}

void RecvProxy_Animation( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	RecvProxy_String( prop, object, out, dgi );

	C_BaseAnimating *self = (C_BaseAnimating *)object;
	self->set_animation( self->_animation );
}

void C_BaseAnimating::anim_blending_func( CEntityThinkFunc *func, void *data )
{
	C_BaseAnimating *self = (C_BaseAnimating *)data;
	if ( self->_playing_anim )
	{
		self->update_blending();
	}
}

void C_BaseAnimating::spawn()
{
	BaseClass::spawn();
	make_think_func( "animblending", anim_blending_func, this );
}

IMPLEMENT_CLIENTCLASS_RT( C_BaseAnimating, DT_BaseAnimating, CBaseAnimating )
	RecvPropString( RECVINFO( _model_path ), RecvProxy_ModelPath ),
	RecvPropVec3( RECVINFO( _model_origin ), RecvProxy_ModelOrigin ),
	RecvPropVec3( RECVINFO( _model_angles ), RecvProxy_ModelAngles ),
	RecvPropVec3( RECVINFO( _model_scale ), RecvProxy_ModelScale ),
	RecvPropString( RECVINFO( _animation ), RecvProxy_Animation )
END_RECV_TABLE()