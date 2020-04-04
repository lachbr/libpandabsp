#include "cl_audiosystem.h"
#include "rendersystem.h"
#include "clientbase.h"

ClientAudioSystem *claudio = nullptr;

IMPLEMENT_CLASS( ClientAudioSystem )

ClientAudioSystem::ClientAudioSystem() :
	AudioSystem()
{
	claudio = this;
}

const char *ClientAudioSystem::get_name() const
{
	return "ClientAudioSystem";
}

bool ClientAudioSystem::initialize()
{
	if ( !BaseClass::initialize() )
	{
		return false;
	}

	cl->get_game_system_of_type( _rsys );
	if ( !_rsys )
	{
		return false;
	}
}

void ClientAudioSystem::update( double frametime )
{
	BaseClass::update( frametime );

	// Update 3D audio listener attributes

	// Get world space view position
	CPT( TransformState ) view_transform = _rsys->get_camera().get_net_transform();
	LPoint3f view_position = view_transform->get_pos();
	// Feet per second
	LVector3f view_velocity = ( view_position - _last_view_position ) / frametime;
	LQuaternionf view_quat = view_transform->get_norm_quat();
	LVector3f view_forward = view_quat.get_forward();
	LVector3f view_up = view_quat.get_up();

	size_t count = _sfx_managers.size();
	for ( size_t i = 0; i < count; i++ )
	{
		AudioManager *mgr = _sfx_managers[i];
		mgr->audio_3d_set_listener_attributes(
			view_position[0], view_position[1], view_position[2],
			view_velocity[0], view_velocity[1], view_velocity[2],
			view_forward[0], view_forward[1], view_forward[2],
			view_up[0], view_up[1], view_up[2] );
	}

	_last_view_position = view_position;
}