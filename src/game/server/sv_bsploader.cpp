#include "sv_bsploader.h"
#include "sv_netinterface.h"
#include "sv_entitymanager.h"
#include "physicssystem.h"
#include "serverbase.h"

#include <texturePool.h>

CSV_BSPLoader::CSV_BSPLoader() :
	BSPLoader()
{
	set_ai( true );
}

void CSV_BSPLoader::load_geometry()
{
	make_faces_ai();
}

void CSV_BSPLoader::cleanup_entities( bool is_transition )
{
	// If we are in a transition to another level, unload any entities
	// that aren't being perserved. Or if we are not in a transition,
	// unload all entities.
	for ( auto itr = _entities.begin(); itr != _entities.end(); )
	{
		WPT( CBaseEntity ) ent = *itr;
		if ( ent.was_deleted() )
			continue;

		if ( ( is_transition && !ent->_preserved ) || !is_transition )
		{
			svents->remove_entity( ent->get_entnum() );
			itr = _entities.erase( itr );
			continue;
		}
		else
		{
			// This entity is being preserved.
			// The entnum is now invalid since the BSP file is changing.
			// This avoids conflicts with future entities from the new BSP file.
			ent->_bsp_entnum = -1;
		}
		itr++;
	}

	if ( !is_transition )
	{
		_entities.clear();
		clear_transition_landmark();
	}
	else if ( !_transition_source_landmark.is_empty() )
	{
		// We are in a transition to another level.
		// Store all entity transforms relative to the source landmark.
		for ( size_t i = 0; i < _entities.size(); i++ )
		{
			WPT( CBaseEntity ) ent = _entities[i];
			if ( ent.was_deleted() )
				continue;

			ent->_landmark_relative_transform = ent->get_node_path().get_mat( _transition_source_landmark );
		}
	}
}

bool CSV_BSPLoader::read( const Filename &file, bool is_transition )
{
	if ( !BSPLoader::read( file, is_transition ) )
		return false;

	spawn_entities();

	if ( is_transition )
	{
		// Find the destination landmark
		CBaseEntity *dest_landmark = nullptr;
		for ( size_t i = 0; i < _entities.size(); i++ )
		{
			WPT( CBaseEntity ) ent = _entities[i];
			if ( ent.was_deleted() )
				continue;

			if ( ent->get_classname() == "info_landmark" &&
			     ent->get_targetname() == _transition_source_landmark.get_name() )
			{
				dest_landmark = ent.p();
				break;
			}
		}

		if ( dest_landmark )
		{
			NodePath dest_landmark_np( "destination_landmark" );
			dest_landmark_np.set_pos( dest_landmark->_origin );
			dest_landmark_np.set_hpr( dest_landmark->_angles );

			for ( size_t i = 0; i < _entities.size(); i++ )
			{
				WPT( CBaseEntity ) ent = _entities[i];
				if ( ent.was_deleted() )
					continue;

				if ( ent->_preserved )
				{
					ent->transition_xform( dest_landmark_np, ent->_landmark_relative_transform );
				}
			}

			dest_landmark_np.remove_node();
		}

		clear_transition_landmark();
	}

	return true;
}

void CSV_BSPLoader::spawn_entities()
{
	for ( size_t i = 0; i < _entities.size(); i++ )
	{
		WPT( CBaseEntity ) ent = _entities[i];
		if ( !ent.was_deleted() )
			ent->spawn();
	}
}

void CSV_BSPLoader::load_entities()
{
	for ( int entnum = 0; entnum < _bspdata->numentities; entnum++ )
	{
		entity_t *ent = &_bspdata->entities[entnum];

		string classname = ValueForKey( ent, "classname" );
		const char *psz_classname = classname.c_str();
		string id = ValueForKey( ent, "id" );

		vec_t origin[3];
		GetVectorDForKey( ent, "origin", origin );

		vec_t angles[3];
		GetVectorDForKey( ent, "angles", angles );

		string targetname = ValueForKey( ent, "targetname" );

		// Explicitly assign worldspawn to entid 0
		bool bexplicit_entnum = strncmp( "worldspawn", psz_classname, 10 ) == 0;
		entid_t explicit_entnum = 0;

		PT( CBaseEntity ) p_ent = svents->make_entity_by_name( classname, false,
								       bexplicit_entnum, explicit_entnum );
		if ( !p_ent )
			continue;

		p_ent->init_mapent( ent, entnum );

		_entities.push_back( p_ent );

	}

}

ServerBSPSystem *svbsp = nullptr;

IMPLEMENT_CLASS( ServerBSPSystem )

ServerBSPSystem::ServerBSPSystem() :
	BaseBSPSystem()
{
	svbsp = this;
}

bool ServerBSPSystem::initialize()
{
	//if ( !BaseClass::initialize() )
	//{
	//	return false;
	//}

	PhysicsSystem *psys;
	sv->get_game_system( psys );

	_bsp_loader = new CSV_BSPLoader;
	_bsp_loader->set_ai( true );
	_bsp_loader->set_physics_world( psys->get_physics_world() );

	return true;
}

void ServerBSPSystem::load_bsp_level( const Filename &path, bool is_transition )
{
	Datagram dg = BeginMessage( NETMSG_CHANGE_LEVEL );
	dg.add_string( path.get_basename_wo_extension() );
	dg.add_bool( is_transition );
	svnet->broadcast_datagram( dg );

	BaseBSPSystem::load_bsp_level( path, is_transition );
	_bsp_level.reparent_to( sv->get_root() );
	_bsp_loader->do_optimizations();
}