/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file entity.cpp
 * @author Brian Lach
 * @date May 03, 2018
 */

#ifdef _PYTHON_VERSION

#include "entity.h"
#include "bsploader.h"
#include "bounding_kdop.h"

entitydef_t::entitydef_t( CBaseEntity *cent, PyObject *pent, bool dyn )
{
	c_entity = cent;
	py_entity = pent;
	preserved = false;
	dynamic = dyn;
}

entitydef_t::~entitydef_t()
{
}

IMPLEMENT_CLASS( CBaseEntity );
IMPLEMENT_CLASS( CPointEntity );
IMPLEMENT_CLASS( CBrushEntity );
IMPLEMENT_CLASS( CBoundsEntity );

///////////////////////////////////// CBaseEntity //////////////////////////////////////////

CBaseEntity::CBaseEntity() :
	TypedReferenceCount(),
	_loader( nullptr ),
	_bsp_entnum( 0 )
{
}

void CBaseEntity::set_data( int entnum, entity_t *ent, BSPLoader *loader )
{
	_loader = loader;

	_bsp_entnum = entnum;

	for ( epair_t * epair = ent->epairs; epair; epair = epair->next )
	{
		_entity_keyvalues[epair->key] = epair->value;
	}
}

BSPLoader *CBaseEntity::get_loader() const
{
	return _loader;
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

/////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// CPointEntity //////////////////////////////////////////

CPointEntity::CPointEntity() :
	CBaseEntity()
{
}

void CPointEntity::set_data( int entnum, entity_t *ent, BSPLoader *loader )
{
	CBaseEntity::set_data( entnum, ent, loader );

	vec3_t pos;
	GetVectorDForKey( ent, "origin", pos );
	_origin = LPoint3( pos[0] / 16.0, pos[1] / 16.0, pos[2] / 16.0 );

	vec3_t angles;
	GetVectorDForKey( ent, "angles", angles );
	_angles = LVector3( angles[1] - 90, angles[0], angles[2] );
}

LPoint3 CPointEntity::get_origin() const
{
	return _origin;
}

LVector3 CPointEntity::get_angles() const
{
	return _angles;
}

/////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// CBoundsEntity /////////////////////////////////////////

CBoundsEntity::CBoundsEntity() :
        CBaseEntity(),
        _mdl( nullptr ),
        _bounds( nullptr )
{
}

BoundingKDOP *CBoundsEntity::get_bounds() const
{
        return _bounds;
}

void CBoundsEntity::fillin_bounds( LPoint3 &mins, LPoint3 &maxs )
{
        mins.set( _mdl->mins[0] / 16.0, _mdl->mins[1] / 16.0, _mdl->mins[2] / 16.0 );
        maxs.set( _mdl->maxs[0] / 16.0, _mdl->maxs[1] / 16.0, _mdl->maxs[2] / 16.0 );
}

bool CBoundsEntity::is_inside( const LPoint3 &pt ) const
{
	int leaf = _loader->find_leaf( pt, _mdl->headnode[0] );
	return leaf == 0;
}

void CBoundsEntity::set_data( int entnum, entity_t *ent, BSPLoader *loader, dmodel_t *mdl )
{
        CBaseEntity::set_data( entnum, ent, loader );
        _mdl = mdl;

	pvector<LPlane> planes;
	for ( int facenum = 0; facenum < mdl->numfaces; facenum++ )
	{
		const dface_t *face = loader->get_bspdata()->dfaces + ( mdl->firstface + facenum );
		const dplane_t *plane = loader->get_bspdata()->dplanes + face->planenum;
		planes.push_back( LPlane( plane->normal[0], plane->normal[1], plane->normal[2], plane->dist / 16.0f ) );
	}
	_bounds = new BoundingKDOP( planes );
}

/////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// CBrushEntity //////////////////////////////////////////

CBrushEntity::CBrushEntity() :
	CBaseEntity()
{
}

void CBrushEntity::set_data( int entnum, entity_t *ent, BSPLoader *loader, dmodel_t *mdl, const NodePath &model )
{
	CBaseEntity::set_data( entnum, ent, loader );
	_mins = LVector3( mdl->mins[0], mdl->mins[1], mdl->mins[2] );
	_maxs = LVector3( mdl->maxs[0], mdl->maxs[1], mdl->maxs[2] );
	_modelnp = model;
}

NodePath CBrushEntity::get_model_np() const
{
	return _modelnp;
}

void CBrushEntity::get_model_bounds( LPoint3 &mins, LPoint3 &maxs )
{
	mins.set( _mins[0] / 16.0, _mins[1] / 16.0, _mins[2] / 16.0 );
	maxs.set( _maxs[0] / 16.0, _maxs[1] / 16.0, _maxs[2] / 16.0 );
}

#endif // #ifdef _PYTHON_VERSION
