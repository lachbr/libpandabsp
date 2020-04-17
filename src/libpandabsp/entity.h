/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file entity.h
 * @author Brian Lach
 * @date May 03, 2018
 */

#ifndef ENTITY_H
#define ENTITY_H

#ifdef _PYTHON_VERSION

#include "config_bsp.h"
#include "bounding_kdop.h"

#ifndef CPPPARSER
#include "bspfile.h"
#endif

#include <nodePath.h>
#include <typedReferenceCount.h>
#include <simpleHashMap.h>

class BSPLoader;
class CBaseEntity;

#ifdef CPPPARSER
class entity_t;
class dmodel_t;
#endif

struct entitydef_t
{
	PT( CBaseEntity ) c_entity;
	PyObject *py_entity;

	bool preserved;
	bool dynamic;

	LMatrix4f landmark_relative_transform;

	entitydef_t( CBaseEntity *cent, PyObject *pent = nullptr, bool dyn = false );
	~entitydef_t();
};

class EXPCL_PANDABSP CBaseEntity : public TypedReferenceCount
{
        DECLARE_CLASS( CBaseEntity, TypedReferenceCount );

PUBLISHED:
	CBaseEntity();

	BSPLoader *get_loader() const;

	INLINE std::string get_entity_value( const std::string &key ) const
	{
		int itr = _entity_keyvalues.find( key );
		if ( itr != -1 )
		{
			return _entity_keyvalues.get_data( itr );
		}
		return "";
	}

	LVector3 get_entity_value_vector( const std::string &key ) const;
	LColor get_entity_value_color( const std::string &key, bool scale = true ) const;

	INLINE std::string get_classname() const
	{
		return get_entity_value( "classname" );
	}
	INLINE std::string get_targetname() const
	{
		return get_entity_value( "targetname" );
	}

	INLINE int get_bsp_entnum() const
	{
		return _bsp_entnum;
	}

public:
	void set_data( int entnum, entity_t *ent, BSPLoader *loader );

protected:
	BSPLoader *_loader;
	// The entity index in the BSP file from which this entity originated.
	// If this entity was preserved in a level transition, this index
	// is no longer valid.
	int _bsp_entnum;

	SimpleHashMap<std::string, std::string, string_hash> _entity_keyvalues;

	friend class Py_AI_BSPLoader;
};

class EXPCL_PANDABSP CPointEntity : public CBaseEntity
{
        DECLARE_CLASS( CPointEntity, CBaseEntity );

PUBLISHED:
	CPointEntity();

	LPoint3 get_origin() const;
	LVector3 get_angles() const;

public:
	void set_data( int entnum, entity_t *ent, BSPLoader *loader );

private:
	LPoint3 _origin;
	LVector3 _angles;
};

/**
 * A flavor of a brush entity (but doesn't inherit from CBrushEntity) which uses the brush only to describe
 * the bounds. Useful for triggers or water, because we don't actually care about the brush's geometry.
 */
class EXPCL_PANDABSP CBoundsEntity : public CBaseEntity
{
        DECLARE_CLASS( CBoundsEntity, CBaseEntity );

PUBLISHED:
        CBoundsEntity();

        BoundingKDOP *get_bounds() const;
	bool is_inside( const LPoint3 &pos ) const;

        void fillin_bounds( LPoint3 &mins, LPoint3 &maxs );

public:
        void set_data( int entnum, entity_t *t, BSPLoader *loader, dmodel_t *mdl );

private:
        PT( BoundingKDOP ) _bounds;
        dmodel_t *_mdl;
};

class EXPCL_PANDABSP CBrushEntity : public CBaseEntity
{
        DECLARE_CLASS( CBrushEntity, CBaseEntity );

PUBLISHED:
	CBrushEntity();
	
	NodePath get_model_np() const;
	void get_model_bounds( LPoint3 &mins, LPoint3 &maxs );

public:
	void set_data( int entnum, entity_t *ent, BSPLoader *loader, dmodel_t *mdl, const NodePath &model );

private:
	LVector3 _mins;
	LVector3 _maxs;
	NodePath _modelnp;
};

#endif // #ifdef _PYTHON_VERSION

#endif // ENTITY_H
