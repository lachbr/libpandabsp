#ifdef _PYTHON_VERSION

#include "py_bsploader.h"
#include "texturePool.h"
#include "sceneGraphReducer.h"

PyObject *Py_BSPLoader::get_entity( int n ) const
{
	PyObject *pent = _entities[n].py_entity;
	Py_INCREF( pent );
	return pent;
}

CBaseEntity *Py_BSPLoader::get_c_entity( const int entnum ) const
{
	for ( size_t i = 0; i < _entities.size(); i++ )
	{
		const entitydef_t &def = _entities[i];
		if ( !def.c_entity )
			continue;
		if ( def.c_entity &&
		     def.c_entity->get_bsp_entnum() == entnum )
		{
			return def.c_entity;
		}
	}

	return nullptr;
}

PyObject *Py_BSPLoader::find_all_entities( const string &classname )
{
	PyObject *list = PyList_New( 0 );

	for ( size_t i = 0; i < _entities.size(); i++ )
	{
		const entitydef_t &def = _entities[i];
		if ( !def.c_entity )
			continue;
		std::string cls = def.c_entity->get_entity_value( "classname" );
		if ( classname == cls )
		{
			PyList_Append( list, def.py_entity );
		}
	}

	Py_INCREF( list );
	return list;
}

void Py_BSPLoader::get_entity_keyvalues( PyObject *list, const int entnum )
{
	entity_t *ent = _bspdata->entities + entnum;
	for ( epair_t *ep = ent->epairs; ep->next != nullptr; ep = ep->next )
	{
		PyObject *kv = PyTuple_New( 2 );
		PyTuple_SetItem( kv, 0, PyString_FromString( ep->key ) );
		PyTuple_SetItem( kv, 1, PyString_FromString( ep->value ) );
		PyList_Append( list, kv );
	}
}

void Py_BSPLoader::link_cent_to_pyent( int entnum, PyObject *pyent )
{
	entitydef_t *pdef = nullptr;
	bool found_pdef = false;

	entity_t *ent = _bspdata->entities + entnum;
	std::string targetname = ValueForKey( ent, "targetname" );

	pvector<entitydef_t *> children;
	children.reserve( 32 );

	for ( size_t i = 0; i < _entities.size(); i++ )
	{
		entitydef_t &def = _entities[i];
		if ( !def.c_entity )
			continue;

		if ( !found_pdef && def.c_entity->get_bsp_entnum() == entnum )
		{
			pdef = &def;
			found_pdef = true;
		}

		if ( def.c_entity->get_bsp_entnum() != entnum &&
		     targetname.length() > 0u &&
		     def.py_entity != nullptr )
		{
			std::string parentname = def.c_entity->get_entity_value( "parent" );
			if ( !parentname.compare( targetname ) )
			{
				// This entity is parented to the specified entity.
				children.push_back( &def );
			}
		}
	}

	nassertv( pdef != nullptr );
	Py_INCREF( pyent );
	pdef->py_entity = pyent;

	for ( size_t i = 0; i < children.size(); i++ )
	{
		entitydef_t *def = children[i];
		PyObject_CallMethod( def->py_entity, "parentGenerated", NULL );
	}
}

PyObject *Py_BSPLoader::get_py_entity_by_target_name( const string &targetname ) const
{
	for ( size_t i = 0; i < _entities.size(); i++ )
	{
		const entitydef_t &def = _entities[i];
		if ( !def.c_entity )
			continue;
		PyObject *pyent = def.py_entity;
		if ( !pyent )
			continue;
		string tname = def.c_entity->get_entity_value( "targetname" );
		if ( tname == targetname )
		{
			Py_INCREF( pyent );
			return pyent;
		}
	}

	Py_RETURN_NONE;
}

/**
 * Manually remove a Python entity from the list.
 * Note: `unload()` will no longer be called on this entity when the level unloads.
 */
void Py_BSPLoader::remove_py_entity( PyObject *obj )
{
	for ( size_t i = 0; i < _entities.size(); i++ )
	{
		entitydef_t &ent = _entities[i];
		if ( ent.py_entity == obj )
		{
			Py_DECREF( ent.py_entity );
			ent.py_entity = nullptr;
		}
	}
}

bool Py_BSPLoader::read( const Filename &filename, bool is_transition )
{
	if ( !BSPLoader::read( filename, is_transition ) )
	{
		return false;
	}

	spawn_entities();

	return true;
}

void Py_BSPLoader::spawn_entities()
{
	// Now load all of the entities at the application level.
	for ( size_t i = 0; i < _entities.size(); i++ )
	{
		entitydef_t &ent = _entities[i];
		if ( ent.py_entity && !ent.dynamic && !ent.preserved )
		{
			// This is a newly loaded (not preserved from previous level) entity
			// that is from the BSP file.
			PyObject_CallMethod( ent.py_entity, "load", NULL );
		}

	}
}

//============================================================================================

PyObject *Py_CL_BSPLoader::make_pyent( PyObject *py_ent, const string &classname )
{
	if ( _entity_to_class.find( classname ) != _entity_to_class.end() )
	{
		// A python class was linked to this entity!
		PyObject *obj = PyObject_CallObject( (PyObject *)_entity_to_class[classname], NULL );
		if ( obj == nullptr )
			PyErr_PrintEx( 1 );
		Py_INCREF( obj );
		// Give the python entity a handle to the c entity.
		PyObject_SetAttrString( obj, "cEntity", py_ent );
		// Precache all resources the entity will use.
		PyObject_CallMethod( obj, "precache", NULL );
		// Don't call load just yet, we need to have all of the entities created first, because some
		// entities rely on others.
		return obj;
	}
	return nullptr;
}

void Py_CL_BSPLoader::link_entity_to_class( const string &entname, PyTypeObject *type )
{
	_entity_to_class[entname] = type;
}

void Py_CL_BSPLoader::load_geometry()
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

void Py_CL_BSPLoader::cleanup_entities( bool is_transition )
{
	// Unload any client-side only entities.
	// Assume the server will take care of unloading networked entities.
	//
	// UNDONE: Client-side only entities are an obsolete feature.
	//         All entities should eventually be networked and client-side
	//         entity functionality should be removed.
	for ( auto itr = _entities.begin(); itr != _entities.end(); itr++ )
	{
		entitydef_t &def = *itr;
		if ( def.py_entity )
		{
			if ( _entity_to_class.find( def.c_entity->get_classname() ) != _entity_to_class.end() )
			{
				// This is a client-sided entity. Unload it.
				PyObject_CallMethod( def.py_entity, "unload", NULL );
			}

			Py_DECREF( def.py_entity );
			def.py_entity = nullptr;
		}
	}
	_entities.clear();
}

void Py_CL_BSPLoader::load_entities()
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

				dmodel_t *mdl = &_bspdata->dmodels[modelnum];

				PT( CBoundsEntity ) entity = new CBoundsEntity;
				entity->set_data( entnum, ent, this, mdl );

				PyObject *py_ent = DTool_CreatePyInstance<CBoundsEntity>( entity, true );
				PyObject *linked = make_pyent( py_ent, classname );

				_entities.push_back( entitydef_t( entity, linked ) );
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

				PT( CBrushEntity ) entity = new CBrushEntity;
				entity->set_data( entnum, ent, this, &_bspdata->dmodels[modelnum], modelroot );

				PyObject *py_ent = DTool_CreatePyInstance<CBrushEntity>( entity, true );
				PyObject *linked = make_pyent( py_ent, classname );

				_entities.push_back( entitydef_t( entity, linked ) );
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

			// We don't know what this entity is exactly, maybe they linked it to a python class.
			// It didn't start with func_, so we can assume it's just a point entity.
			PT( CPointEntity ) entity = new CPointEntity;
			entity->set_data( entnum, ent, this );

			PyObject *py_ent = DTool_CreatePyInstance<CPointEntity>( entity, true );
			PyObject *linked = make_pyent( py_ent, classname );

			_entities.push_back( entitydef_t( entity, linked ) );
		}
	}
}

//============================================================================================

void Py_AI_BSPLoader::add_dynamic_entity( PyObject *pyent )
{
	Py_INCREF( pyent );
	_entities.push_back( entitydef_t( nullptr, pyent, true ) );
}

void Py_AI_BSPLoader::remove_dynamic_entity( PyObject *pyent )
{
	auto itr = _entities.end();
	for ( size_t i = 0; i < _entities.size(); i++ )
	{
		const entitydef_t &def = _entities[i];
		if ( def.py_entity == pyent )
		{
			itr = _entities.begin() + i;
			break;
		}
	}

	nassertv( itr != _entities.end() );

	Py_DECREF( pyent );
	_entities.erase( itr );
}

void Py_AI_BSPLoader::mark_entity_preserved( int n, bool preserved )
{
	_entities[n].preserved = preserved;
}

void Py_AI_BSPLoader::set_server_entity_dispatcher( PyObject *dispatch )
{
	_sv_ent_dispatch = dispatch;
}

void Py_AI_BSPLoader::link_server_entity_to_class( const string &name, PyTypeObject *type )
{
	_svent_to_class[name] = type;
}

void Py_AI_BSPLoader::load_entities()
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

		if ( _svent_to_class.find( classname ) != _svent_to_class.end() )
		{
			if ( _sv_ent_dispatch != nullptr )
			{
				PyObject *ret = PyObject_CallMethod( _sv_ent_dispatch, "createServerEntity",
									"Oi", _svent_to_class[classname], entnum );
				if ( !ret )
				{
					PyErr_PrintEx( 1 );
				}
				else
				{
					PT( CBaseEntity ) entity;
					if ( !strncmp( psz_classname, "func_", 5 ) ||
						entnum == 0 )
					{
						int modelnum;
						if ( entnum == 0 )
							modelnum = 0;
						else
							modelnum = extract_modelnum( entnum );
						entity = new CBrushEntity;
						DCAST( CBrushEntity, entity )->set_data( entnum, ent, this,
												_bspdata->dmodels + modelnum, get_model( modelnum ) );
					}
					else if ( !strncmp( classname.c_str(), "trigger_", 8 ) )
					{
						int modelnum = extract_modelnum_s( ent );
						entity = new CBoundsEntity;
						DCAST( CBoundsEntity, entity )->set_data( entnum, ent, this,
												&_bspdata->dmodels[modelnum] );
					}
					else
					{
						entity = new CPointEntity;
						DCAST( CPointEntity, entity )->set_data( entnum, ent, this );
					}

					Py_INCREF( ret );
					_entities.push_back( entitydef_t( entity, ret ) );
				}
			}
		}

	}
}

bool Py_AI_BSPLoader::read( const Filename &filename, bool is_transition )
{
	if ( !Py_BSPLoader::read( filename, is_transition ) )
	{
		return false;
	}

	if ( is_transition )
	{
		// Find the destination landmark
		entitydef_t *dest_landmark = nullptr;
		for ( size_t i = 0; i < _entities.size(); i++ )
		{
			entitydef_t &def = _entities[i];
			if ( !def.c_entity )
				continue;
			if ( def.c_entity->get_classname() == "info_landmark" &&
			     def.c_entity->get_targetname() == _transition_source_landmark.get_name() )
			{
				dest_landmark = &def;
				break;
			}
		}

		if ( dest_landmark )
		{
			CPointEntity *dest_ent = DCAST( CPointEntity, dest_landmark->c_entity );
			NodePath dest_landmark_np( "destination_landmark" );
			dest_landmark_np.set_pos( dest_ent->get_origin() );
			dest_landmark_np.set_hpr( dest_ent->get_angles() );
			PyObject *py_dest_landmark_np =
				DTool_CreatePyInstance( &dest_landmark_np, *(Dtool_PyTypedObject *)dest_landmark_np.get_class_type().get_python_type(), true, true );
			Py_INCREF( py_dest_landmark_np );

			for ( size_t i = 0; i < _entities.size(); i++ )
			{
				entitydef_t &def = _entities[i];
				if ( def.preserved && def.py_entity )
				{
					LMatrix4f mat = def.landmark_relative_transform;
					PyObject *py_mat = DTool_CreatePyInstance( &mat, *(Dtool_PyTypedObject *)mat.get_class_type().get_python_type(), true, true );
					Py_INCREF( py_mat );

					PyObject_CallMethod( def.py_entity, "transitionXform", "OO", py_dest_landmark_np, py_mat );

					Py_DECREF( py_mat );
				}
			}

			Py_DECREF( py_dest_landmark_np );
			dest_landmark_np.remove_node();
		}

		clear_transition_landmark();
	}

	return true;
}

void Py_AI_BSPLoader::load_geometry()
{
	make_faces_ai();
}

void Py_AI_BSPLoader::cleanup_entities( bool is_transition )
{
	// If we are in a transition to another level, unload any entities
	// that aren't being perserved. Or if we are not in a transition,
	// unload all entities.
	for ( auto itr = _entities.begin(); itr != _entities.end(); )
	{
		entitydef_t &def = *itr;
		if ( ( is_transition && !def.preserved ) || !is_transition )
		{
			PyObject *py_ent = def.py_entity;
			if ( py_ent )
			{
				PyObject_CallMethod( py_ent, "unload", NULL );
				Py_DECREF( py_ent );
				def.py_entity = nullptr;
			}
			itr = _entities.erase( itr );
			continue;
		}
		else if ( def.c_entity )
		{
			// This entity is being preserved.
			// The entnum is now invalid since the BSP file is changing.
			// This avoids conflicts with future entities from the new BSP file.
			def.c_entity->_bsp_entnum = -1;
		}
		itr++;
	}

	if ( !is_transition )
	{
		_entities.clear();
	}
	else if ( !_transition_source_landmark.is_empty() )
	{
		// We are in a transition to another level.
		// Store all entity transforms relative to the source landmark.
		for ( size_t i = 0; i < _entities.size(); i++ )
		{
			entitydef_t &ent = _entities[i];
			if ( ent.py_entity && DtoolInstance_Check( ent.py_entity ) )
			{
				NodePath *pyent_np = (NodePath *)
					DtoolInstance_VOID_PTR( ent.py_entity );
				if ( pyent_np )
				{
					ent.landmark_relative_transform =
						pyent_np->get_mat( _transition_source_landmark );
				}
			}
		}
	}
}

#endif // #ifdef _PYTHON_VERSION