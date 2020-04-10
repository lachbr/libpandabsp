#include "baseentity.h"
#include "serverbase.h"
#include "sv_entitymanager.h"

#include <modelRoot.h>
#include <bulletBoxShape.h>

CBaseEntity::CBaseEntity() :
	CBaseEntityShared(),
	_map_entity( false ),
	_preserved( false ),
	_owner_client( 0 )
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

	_bsp_entnum = -1;
}

void CBaseEntity::spawn()
{
	CBaseEntityShared::spawn();

	//_np.reparent_to( sv->get_root() );
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

PT( CBaseEntity ) CreateEntityByName( const std::string &name )
{
	return svents->make_entity_by_name( name );
}