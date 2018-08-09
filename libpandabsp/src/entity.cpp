#include "entity.h"
#include "bsploader.h"

TypeHandle CBaseEntity::_type_handle;
TypeHandle CPointEntity::_type_handle;
TypeHandle CBrushEntity::_type_handle;
TypeHandle CBoundsEntity::_type_handle;

///////////////////////////////////// CBaseEntity //////////////////////////////////////////

CBaseEntity::CBaseEntity() :
	TypedReferenceCount(),
	_ent( nullptr ),
	_entnum( 0 ),
	_loader( nullptr )
{
}


void CBaseEntity::set_data( int entnum, entity_t *ent, BSPLoader *loader )
{
	_entnum = entnum;
	_ent = ent;
	_loader = loader;
}

int CBaseEntity::get_entnum() const
{
	return _entnum;
}

BSPLoader *CBaseEntity::get_loader() const
{
	return _loader;
}

/////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// CPointEntity //////////////////////////////////////////

CPointEntity::CPointEntity() :
	CBaseEntity()
{
}

LPoint3 CPointEntity::get_origin() const
{
	vec3_t pos;
	GetVectorForKey( _ent, "origin", pos );
	return LPoint3( pos[0] / 16.0, pos[1] / 16.0, pos[2] / 16.0 );
}

LVector3 CPointEntity::get_angles() const
{
	vec3_t angles;
	GetVectorForKey( _ent, "angles", angles );
	return LVector3( angles[1] - 90, angles[0], angles[2] );
}

/////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// CBoundsEntity /////////////////////////////////////////

CBoundsEntity::CBoundsEntity() :
        CBaseEntity(),
        _mdl( nullptr ),
        _bounds( nullptr )
{
}

INLINE bool CBoundsEntity::is_inside( const LPoint3 &pos ) const
{
        return _bounds->contains( pos ) != BoundingVolume::IF_no_intersection;
}

BoundingBox *CBoundsEntity::get_bounds() const
{
        return _bounds;
}

void CBoundsEntity::fillin_bounds( LPoint3 &mins, LPoint3 &maxs )
{
        mins.set( _mdl->mins[0] / 16.0, _mdl->mins[1] / 16.0, _mdl->mins[2] / 16.0 );
        maxs.set( _mdl->maxs[0] / 16.0, _mdl->maxs[1] / 16.0, _mdl->maxs[2] / 16.0 );
}

void CBoundsEntity::set_data( int entnum, entity_t *ent, BSPLoader *loader, dmodel_t *mdl )
{
        CBaseEntity::set_data( entnum, ent, loader );
        _mdl = mdl;
        _bounds = new BoundingBox( LPoint3( mdl->mins[0] / 16.0, mdl->mins[1] / 16.0, mdl->mins[2] / 16.0 ),
                                   LPoint3( mdl->maxs[0] / 16.0, mdl->maxs[1] / 16.0, mdl->maxs[2] / 16.0 ) );
}

/////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// CBrushEntity //////////////////////////////////////////

CBrushEntity::CBrushEntity() :
	CBaseEntity(),
	_modelnum( 0 ),
	_mdl( nullptr ) 
{
}

void CBrushEntity::set_data( int entnum, entity_t *ent, BSPLoader *loader, int modelnum, dmodel_t *mdl, const NodePath &model )
{
	_entnum = entnum;
	_ent = ent;
	_modelnum = modelnum;
	_mdl = mdl;
	_modelnp = model;
	_loader = loader;
}



NodePath CBrushEntity::get_model_np() const
{
	return _modelnp;
}

int CBrushEntity::get_modelnum() const
{
	return _modelnum;
}

void CBrushEntity::get_model_bounds( LPoint3 &mins, LPoint3 &maxs )
{
	mins.set( _mdl->mins[0] / 16.0, _mdl->mins[1] / 16.0, _mdl->mins[2] / 16.0 );
	maxs.set( _mdl->maxs[0] / 16.0, _mdl->maxs[1] / 16.0, _mdl->maxs[2] / 16.0 );
}