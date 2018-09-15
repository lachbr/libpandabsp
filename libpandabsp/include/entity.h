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
        TypeDecl( CBaseEntity, TypedReferenceCount );

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
};

class EXPCL_PANDABSP CPointEntity : public CBaseEntity
{
        TypeDecl( CPointEntity, CBaseEntity );

PUBLISHED:
	CPointEntity();

	LPoint3 get_origin() const;
	LVector3 get_angles() const;
};

/**
 * A flavor of a brush entity (but doesn't inherit from CBrushEntity) which uses the brush only to describe
 * the bounds. Useful for triggers or water, because we don't actually care about the brush's geometry.
 */
class EXPCL_PANDABSP CBoundsEntity : public CBaseEntity
{
        TypeDecl( CBoundsEntity, CBaseEntity );

PUBLISHED:
        CBoundsEntity();

        BoundingBox *get_bounds() const;
        INLINE bool is_inside( const LPoint3 &pos ) const;

        void fillin_bounds( LPoint3 &mins, LPoint3 &maxs );

public:
        void set_data( int entnum, entity_t *t, BSPLoader *loader, dmodel_t *mdl );

private:
        PT( BoundingBox ) _bounds;
        dmodel_t *_mdl;
};

class EXPCL_PANDABSP CBrushEntity : public CBaseEntity
{
        TypeDecl( CBrushEntity, CBaseEntity );

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
};

#endif // ENTITY_H