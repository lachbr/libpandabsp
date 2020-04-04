#include "cl_bsploader.h"
#include "clientbase.h"
#include "cl_rendersystem.h"
#include "physicssystem.h"
#include "cl_netinterface.h"
#include "netmessages.h"

#include <texturePool.h>
#include <sceneGraphReducer.h>

CL_BSPLoader::CL_BSPLoader() :
	BSPLoader()
{
	set_ai( false );
}

void CL_BSPLoader::load_geometry()
{
	load_cubemaps();

	LightmapPalettizer lmp( this );
	_lightmap_dir = lmp.palettize_lightmaps();

	make_faces();
	SceneGraphReducer gr;
	gr.apply_attribs( _result.node() );

	_result.set_attrib( BSPFaceAttrib::make_default(), 1 );

	_decal_mgr.init();
}

void CL_BSPLoader::load_entities()
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

		if ( !strncmp( classname.c_str(), "trigger_", 8 ) ||
			!strncmp( classname.c_str(), "func_water", 10 ) )
		{
			// This is a bounds entity. We do not actually care about the geometry,
			// but the mins and maxs of the model. We will use that to create
			// a BoundingBox to check if the avatar is inside of it.
			int modelnum = extract_modelnum_s( ent );
			if ( modelnum != -1 )
			{
				remove_model( modelnum );
				_model_data[modelnum].model_root = get_model( 0 );
				_model_data[modelnum].merged_modelnum = 0;
				NodePath( _model_data[modelnum].decal_rbc ).remove_node();
				_model_data[modelnum].decal_rbc = nullptr;
			}
		}
		else if ( !strncmp( classname.c_str(), "func_", 5 ) )
		{
			// Brush entites begin with func_, handle those accordingly.
			int modelnum = extract_modelnum_s( ent );
			if ( modelnum != -1 )
			{
				// Brush model
				NodePath modelroot = get_model( modelnum );
				// render all faces of brush model in a single batch
				clear_model_nodes_below( modelroot );
				modelroot.flatten_strong();

				if ( !strncmp( classname.c_str(), "func_wall", 9 ) ||
					!strncmp( classname.c_str(), "func_detail", 11 ) ||
					!strncmp( classname.c_str(), "func_illusionary", 17 ) )
				{
					// func_walls and func_details aren't really entities,
					// they are just hints to the compiler. we can treat
					// them as regular brushes part of worldspawn.
					modelroot.wrt_reparent_to( get_model( 0 ) );
					flatten_node( modelroot );
					NodePathCollection npc = modelroot.get_children();
					for ( int n = 0; n < npc.get_num_paths(); n++ )
					{
						npc[n].wrt_reparent_to( get_model( 0 ) );
					}
					remove_model( modelnum );
					_model_data[modelnum].model_root = _model_data[0].model_root;
					NodePath( _model_data[modelnum].decal_rbc ).remove_node();
					_model_data[modelnum].decal_rbc = _model_data[0].decal_rbc;
					_model_data[modelnum].merged_modelnum = 0;
					continue;
				}
			}
		}
		else if ( !strncmp( classname.c_str(), "infodecal", 9 ) )
		{
			const char *mat = ValueForKey( ent, "texture" );
			const BSPMaterial *bspmat = BSPMaterial::get_from_file( mat );
			Texture *tex = TexturePool::load_texture( bspmat->get_keyvalue( "$basetexture" ) );
			LPoint3 vpos( origin[0], origin[1], origin[2] );
			_decal_mgr.decal_trace( mat, LPoint2( tex->get_orig_file_x_size() / 16.0, tex->get_orig_file_y_size() / 16.0 ),
						0.0, vpos, vpos, LColorf( 1.0 ), DECALFLAGS_STATIC );
		}
		else
		{
			if ( classname == "light_environment" )
				_light_environment = ent;
		}
	}
}

ClientBSPSystem *clbsp = nullptr;

IMPLEMENT_CLASS( ClientBSPSystem )

ClientBSPSystem::ClientBSPSystem() :
	BaseBSPSystem()
{
	clbsp = this;
}

bool ClientBSPSystem::initialize()
{
	ClientRenderSystem *rsys;
	cl->get_game_system( rsys );
	PhysicsSystem *psys;
	cl->get_game_system( psys );

	_bsp_loader = new CL_BSPLoader;
	_bsp_loader->set_gamma( 2.2f );
	_bsp_loader->set_render( rsys->get_render() );
	_bsp_loader->set_want_visibility( true );
	_bsp_loader->set_want_lightmaps( true );
	_bsp_loader->set_visualize_leafs( false );
	_bsp_loader->set_physics_world( psys->get_physics_world() );
	_bsp_loader->set_win( rsys->get_graphics_window() );
	_bsp_loader->set_camera( rsys->get_camera() );
	BSPLoader::set_global_ptr( _bsp_loader );

	_bsp_loader->set_shader_generator( clrender->get_shader_generator() );

	return true;
}

void ClientBSPSystem::load_bsp_level( const Filename &path, bool is_transition )
{
	ClientRenderSystem *rsys;
	ClientNetInterface *nsys;
	PhysicsSystem *psys;
	cl->get_game_system( rsys );
	cl->get_game_system( nsys );
	cl->get_game_system( psys );

	nsys->set_client_state( CLIENTSTATE_LOADING );

	BaseBSPSystem::load_bsp_level( path, is_transition );
	_bsp_loader->do_optimizations();
	NodePathCollection props = _bsp_level.find_all_matches( "**/+BSPProp" );
	for ( int i = 0; i < props.get_num_paths(); i++ )
	{
		psys->create_and_attach_bullet_nodes( props[i] );
	}

	_bsp_level.reparent_to( rsys->get_render() );

	nsys->set_client_state( CLIENTSTATE_PLAYING );
}

void ClientBSPSystem::cleanup_bsp_level()
{
	if ( !_bsp_level.is_empty() )
	{
		PhysicsSystem *psys;
		cl->get_game_system( psys );
		psys->detach_and_remove_bullet_nodes( _bsp_level );

		_bsp_level.remove_node();
	}
	_bsp_loader->cleanup();
}

bool ClientBSPSystem::handle_datagram( int msgtype, DatagramIterator &dgi )
{
	switch ( msgtype )
	{
	case NETMSG_CHANGE_LEVEL:
		{
			std::string mapname = dgi.get_string();
			bool is_transition = dgi.get_bool();
			load_bsp_level( get_map_filename( mapname ), is_transition );
			return true;
		}
	}

	return false;
}
