/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file py_bsploader.h
 * @author Brian Lach
 * @date April 17, 2020
 *
 * @desc Implements BSPLoader code that is specific to the Python version.
 *
 */

#pragma once

#ifdef _PYTHON_VERSION

#include "bsploader.h"

#include <py_panda.h>
#include "entity.h"

class Py_BSPLoader : public BSPLoader
{
PUBLISHED:
	PyObject *find_all_entities( const string &classname );

	int get_num_entities() const; //
	PyObject *get_entity( int n ) const; //

	CBaseEntity *get_c_entity( const int entnum ) const; //
	void get_entity_keyvalues( PyObject *list, const int entnum ); //
	void link_cent_to_pyent( int entum, PyObject *pyent ); //
	PyObject *get_py_entity_by_target_name( const string &targetname ) const; //

	void spawn_entities();

	void remove_py_entity( PyObject *ent );

	virtual bool read( const Filename &filename, bool is_transition = false );

protected:
	pvector<entitydef_t> _entities;
};

class Py_CL_BSPLoader : public Py_BSPLoader
{
PUBLISHED:
	Py_CL_BSPLoader();

	void link_entity_to_class( const string &entname, PyTypeObject *type );
	PyObject *make_pyent( PyObject *pyent, const string &classname );

protected:
	virtual void load_geometry();
	virtual void cleanup_entities( bool is_transition );
	virtual void load_entities();

private:
	// for purely client-sided, non networked entities
	//
	// utilized only by the client
	pmap<string, PyTypeObject *> _entity_to_class;
};

class Py_AI_BSPLoader : public Py_BSPLoader
{
PUBLISHED:
	Py_AI_BSPLoader();

	void add_dynamic_entity( PyObject *pyent );
	void remove_dynamic_entity( PyObject *pyent );
	void mark_entity_preserved( int n, bool preserved = true );

	void set_server_entity_dispatcher( PyObject *dispatcher );
	void link_server_entity_to_class( const string &name, PyTypeObject *type );

	INLINE void set_transition_landmark( const std::string &name,
					     const LVector3 &origin,
					     const LVector3 &angles )
	{
		// Preserved entity transforms are stored relative to this node,
		// and applied relative to the destination landmark in the new level.
		_transition_source_landmark = NodePath( name );
		_transition_source_landmark.set_pos( origin );
		_transition_source_landmark.set_hpr( angles );
	}
	INLINE void clear_transition_landmark()
	{
		_transition_source_landmark = NodePath();
	}

	virtual bool read( const Filename &filename, bool is_transition = false );

protected:
	virtual void load_geometry();
	virtual void cleanup_entities( bool is_transition );
	virtual void load_entities();

private:
	// entity name to distributed object class
	// createServerEntity is called on the dispatcher object
	// to generate the networked entity
	//
	// utilized only by the AI
	PyObject *_sv_ent_dispatch;
	pmap<string, PyTypeObject *> _svent_to_class;

	NodePath _transition_source_landmark;
	NodePath _transition_dest_landmark;
};

INLINE int Py_BSPLoader::get_num_entities() const
{
	return _entities.size();
}

INLINE Py_CL_BSPLoader::Py_CL_BSPLoader() :
	Py_BSPLoader()
{
}

INLINE Py_AI_BSPLoader::Py_AI_BSPLoader() :
	Py_BSPLoader()
{
}

#endif
