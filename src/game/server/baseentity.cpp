#include "baseentity.h"
#include "serverbase.h"
#include "sv_entitymanager.h"

#include <modelRoot.h>
#include <bulletBoxShape.h>

CBaseEntity::CBaseEntity() :
	CBaseEntityShared(),
	_map_entity( false ),
	_preserved( false ),
	_kinematic( false ),
	_mass( 0.0f ),
	_solid( SOLID_NONE ),
	_bodynode( nullptr ),
	_phys_mask( BitMask32::all_on() ),
	_physics_setup( false ),
	_state( 0 ),
	_state_time( 0.0f ),
	_owner_client( 0 )
{
}

void CBaseEntity::set_state( int state )
{
	_state = state;
	_state_time = _simulation_time;
}

PT( CIBulletRigidBodyNode ) CBaseEntity::get_phys_body()
{
	PT( CIBulletRigidBodyNode ) bnode = new CIBulletRigidBodyNode( "entity-phys" );
	bnode->set_mass( _mass );

	bnode->set_ccd_motion_threshold( 1e-7 );
	bnode->set_ccd_swept_sphere_radius( 0.5f );

	switch ( _solid )
	{
	case SOLID_MESH:
		{
			CollisionNode *cnode = DCAST( CollisionNode, _np.find( "**/+CollisionNode" ).node() );
			PT( BulletConvexHullShape ) convex_hull = PhysicsSystem::
				make_convex_hull_from_collision_solids( cnode );
			bnode->add_shape( convex_hull );
			break;
		}
	case SOLID_BBOX:
		{
			LPoint3 mins, maxs;
			_np.calc_tight_bounds( mins, maxs );
			LVector3f extents = PhysicsSystem::extents_from_min_max( mins, maxs );
			CPT( TransformState ) ts_center = TransformState::make_pos(
				PhysicsSystem::center_from_min_max( mins, maxs ) );
			PT( BulletBoxShape ) shape = new BulletBoxShape( extents );
			bnode->add_shape( shape, ts_center );
			break;
		}
	case SOLID_SPHERE:
		{
			break;
		}
	}

	return bnode;
}

void CBaseEntity::init_physics()
{
	if ( _solid == SOLID_NONE )
	{
		return;
	}

	bool underneath_self = !_kinematic;
	_bodynode = get_phys_body();
	_bodynode->set_user_data( this );
	_bodynode->set_kinematic( false );

	NodePath parent = _np.get_parent();
	_bodynp = parent.attach_new_node( _bodynode );
	_bodynp.set_collide_mask( _phys_mask );
	if ( !underneath_self )
	{
		_np.reparent_to( _bodynp );
		_np = _bodynp;
	}
	else
	{
		_bodynp.reparent_to( _np );
	}

	PhysicsSystem *phys;
	sv->get_game_system( phys );
	phys->get_physics_world()->attach( _bodynode );

	//sv->get_root().ls();

	_physics_setup = true;
}

void CBaseEntity::transition_xform( const NodePath &landmark_np, const LMatrix4 &transform )
{
	
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
	_owner_entity = 0;
}

void CBaseEntity::spawn()
{
	CBaseEntityShared::spawn();

	_np.reparent_to( sv->get_root() );
}

void CBaseEntity::despawn()
{
	if ( _physics_setup )
	{
		PhysicsSystem *phys;
		sv->get_game_system( phys );

		phys->get_physics_world()->remove( _bodynode );
		_bodynp.remove_node();
		_bodynode = nullptr;
		_physics_setup = false;
	}

	CBaseEntityShared::despawn();
}

void CBaseEntity::simulate()
{
	_simulation_time = sv->get_curtime();

	if ( _physics_setup && _kinematic )
	{
		_origin = _bodynp.get_pos();
		_angles = _bodynp.get_hpr();
		//std::cout << _origin << " | " << _angles << std::endl;
	}
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

void CBaseEntity::receive_entity_message( int msgtype, uint32_t client_id, DatagramIterator &dgi )
{
}

void CBaseEntity::send_entity_message( Datagram &dg )
{
	svnet->broadcast_datagram( dg, true );
}

void CBaseEntity::send_entity_message( Datagram &dg, const vector_uint32 &client_ids )
{
	svnet->send_datagram_to_clients( dg, client_ids );
}



IMPLEMENT_SERVERCLASS_ST_NOBASE( CBaseEntity )
	SendPropInt( PROPINFO( _bsp_entnum ) ),
	SendPropEntnum( PROPINFO( _parent_entity ) ),
	SendPropVec3( PROPINFO( _origin ) ),
	SendPropVec3( PROPINFO( _angles ) ),
	SendPropVec3( PROPINFO( _scale ) ),
	SendPropInt( PROPINFO( _simulation_tick ) ),
	SendPropEntnum( PROPINFO( _owner_entity ) ) 
END_SEND_TABLE()

PT( CBaseEntity ) CreateEntityByName( const std::string &name )
{
	return svents->make_entity_by_name( name );
}