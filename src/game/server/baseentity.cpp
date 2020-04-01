#include "baseentity.h"
#include "server.h"
#include "clockdelta.h"
#include "globalvars_server.h"
#include "basegame.h"

#include <modelRoot.h>

CBaseEntity::CBaseEntity() :
	CBaseEntityShared(),
	_map_entity( false ),
	_preserved( false ),
	_num_changed_offsets( MAX_CHANGED_OFFSETS )
{
}

void CBaseEntity::transition_xform( const NodePath &landmark_np, const LMatrix4 &transform )
{
	_np.set_mat( landmark_np, transform );
	_origin = _np.get_pos();
	_angles = _np.get_hpr();
	_scale = _np.get_scale();
}

LVector3 CBaseEntity::get_entity_value_vector( const std::string &key ) const
{
	double          v1, v2, v3;
	LVector3	vec;

	// scanf into doubles, then assign, so it is vec_t size independent
	v1 = v2 = v3 = 0;
	sscanf( get_entity_value( key ).c_str(), "%lf %lf %lf", &v1, &v2, &v3 );
	vec[0] = v1;
	vec[1] = v2;
	vec[2] = v3;
	return vec;
}

LColor CBaseEntity::get_entity_value_color( const std::string &key, bool scale ) const
{
	return color_from_value( get_entity_value( key ), scale, true );
}

void CBaseEntity::init_mapent( entity_t *ent, int bsp_entnum )
{
	_bsp_entnum = bsp_entnum;

	for ( epair_t * epair = ent->epairs; epair; epair = epair->next )
	{
		_entity_keyvalues[epair->key] = epair->value;
	}
}

bool CBaseEntity::can_transition() const
{
	return true;
}

/**
 * Checks if the specified SendProp has been changed since
 * the last delta snapshot. Changed properties are stored
 * by their memory offset from the start of the class.
 */
bool CBaseEntity::is_property_changed( SendProp *prop )
{
	// Assume the property has changed if the entity
	// is marked fully changed.
	if ( is_entity_fully_changed() )
		return true;

	// If the offset of this prop is in the changed array,
	// it has been changed.
	size_t offset = prop->get_offset();
	for ( int i = 0; i < _num_changed_offsets; i++ )
		if ( _changed_offsets[i] == offset )
			return true;

	return false;
}

/**
 * Mark the entity as being fully changed.
 * All properties will be sent in the next snapshot.
 */
void CBaseEntity::network_state_changed()
{
	// Just say that we have the max number of changed offsets
	// to force each property to be sent.
	_num_changed_offsets = MAX_CHANGED_OFFSETS;
}

/**
 * Called when the value of a NetworkVar on this entity has changed.
 * We keep track of changed properties by the variable's memory offset
 * from the start of the class.
 */
void CBaseEntity::network_state_changed( void *ptr )
{
	unsigned short offset = (char *)ptr - (char *)this;

	// Already full? Don't store offset
	if ( is_entity_fully_changed() )
		return;

	// Make sure we don't already have this offset
	for ( int i = 0; i < _num_changed_offsets; i++ )
		if ( _changed_offsets[i] == offset )
			return;

	// This property will be sent in the next delta snapshot.
	_changed_offsets[_num_changed_offsets++] = offset;
}

void CBaseEntity::init( entid_t entnum )
{
	CBaseEntityShared::init( entnum );

	_parent_entity = 0;
	_origin = LVector3( 0 );
	_angles = LVector3( 0 );
	_scale = LVector3( 1 );
	_bsp_entnum = -1;
	_simulation_tick = 0;
	_simulation_time = 0.0f;
}

void CBaseEntity::spawn()
{
	CBaseEntityShared::spawn();
}

void CBaseEntity::simulate()
{
}

void CBaseEntity::set_origin( const LPoint3 &origin )
{
	_origin = origin;
	_np.set_pos( origin );
}

void CBaseEntity::set_angles( const LVector3 &angles )
{
	_angles = angles;
	_np.set_hpr( angles );
}

void CBaseEntity::set_scale( const LVector3 &scale )
{
	_scale = scale;
	_np.set_scale( scale );
}

void SendProxy_SimulationTime( SendProp *prop, void *object, void *data, Datagram &out )
{
	CBaseEntity *ent = (CBaseEntity *)object;

	int ticknumber = g_globals->game->time_to_ticks( ent->_simulation_time );
	// tickbase is current tick rounded down to closest 100 ticks
	int tickbase =
		g_globals->get_network_base( g_globals->tickcount, ent->get_entnum() );
	int addt = 0;
	if ( ticknumber >= tickbase )
	{
		addt = ( ticknumber - tickbase ) & 0xFF;
	}

	out.add_int32( addt );
}

IMPLEMENT_SERVERCLASS_ST_NOBASE( CBaseEntity, DT_BaseEntity, baseentity )
	SendPropInt( SENDINFO( _bsp_entnum ) ),
	SendProp( SENDINFO( _simulation_time ), 0, SendProxy_SimulationTime ),
	SendPropEntnum( SENDINFO( _parent_entity ) ),
	SendPropVec3( SENDINFO( _origin ) ),
	SendPropVec3( SENDINFO( _angles ) ),
	SendPropVec3( SENDINFO( _scale ) ),
	SendPropInt( SENDINFO( _simulation_tick ) )
END_SEND_TABLE()

PT( CBaseEntity ) CreateEntityByName( const std::string &name )
{
	return g_server->make_entity_by_name( name );
}