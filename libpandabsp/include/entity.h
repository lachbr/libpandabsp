#ifndef ENTITY_H
#define ENTITY_H

#include "config_bsp.h"

#ifndef CPPPARSER
#include "bspfile.h"
#endif

#include <nodePath.h>
#include <typedReferenceCount.h>

class BSPLoader;

#ifdef CPPPARSER
class entity_t;
class dmodel_t;
#endif

class EXPCL_PANDABSP CBaseEntity : public TypedReferenceCount
{
PUBLISHED:
	CBaseEntity();

	int get_entnum() const;
	BSPLoader *get_loader() const;

public:
	void set_data( int entnum, entity_t *ent, BSPLoader *loader );

protected:
	entity_t *_ent;
	int _entnum;
	BSPLoader *_loader;

public:
	static TypeHandle get_class_type()
	{
		return _type_handle;
	}
	static void init_type()
	{
		register_type( _type_handle, "CBaseEntity", TypedReferenceCount::get_class_type() );
	}

private:
	static TypeHandle _type_handle;
};

class EXPCL_PANDABSP CPointEntity : public CBaseEntity
{
PUBLISHED:
	CPointEntity();

	LPoint3 get_origin() const;
	LVector3 get_angles() const;

public:
	static TypeHandle get_class_type()
	{
		return _type_handle;
	}
	static void init_type()
	{
		register_type( _type_handle, "CPointEntity", CBaseEntity::get_class_type() );
	}

private:
	static TypeHandle _type_handle;
};

/**
 * A flavor of a brush entity (but doesn't inherit from CBrushEntity) which uses the brush only to describe
 * the bounds. Useful for triggers or water, because we don't actually care about the brush's geometry.
 */
class EXPCL_PANDABSP CBoundsEntity : public CBaseEntity
{
PUBLISHED:
        CBoundsEntity();

        BoundingBox *get_bounds() const;
        INLINE bool is_inside( const LPoint3 &pos ) const;

public:
        void set_data( int entnum, entity_t *t, BSPLoader *loader, dmodel_t *mdl );

private:
        PT( BoundingBox ) _bounds;
        dmodel_t *_mdl;

public:
        static TypeHandle get_class_type()
        {
                return _type_handle;
        }
        static void init_type()
        {
                register_type( _type_handle, "CBoundsEntity", CBaseEntity::get_class_type() );
        }

private:
        static TypeHandle _type_handle;
};

class EXPCL_PANDABSP CBrushEntity : public CBaseEntity
{
PUBLISHED:
	CBrushEntity();
	
	int get_modelnum() const;
	NodePath get_model_np() const;
	void get_model_bounds( LPoint3 &mins, LPoint3 &maxs );

public:
	void set_data( int entnum, entity_t *ent, BSPLoader *loader, int modelnum, dmodel_t *mdl, const NodePath &model );

private:
	
	int _modelnum;
	dmodel_t *_mdl;
	NodePath _modelnp;
	

public:
	static TypeHandle get_class_type()
	{
		return _type_handle;
	}
	static void init_type()
	{
		register_type( _type_handle, "CBrushEntity", CBaseEntity::get_class_type() );
	}

private:
	static TypeHandle _type_handle;
};

#endif // ENTITY_H