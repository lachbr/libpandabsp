/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file decals.cpp
 * @author Brian Lach
 * @date March 08, 2019
 *
 * @desc Decals on the world! TODO: clip the verts
 *
 */

#include "decals.h"
#include "bsp_trace.h"
#include "bsploader.h"
#include "bsp_material.h"
#include "shader_generator.h"

#include <geomVertexFormat.h>
#include <geomVertexData.h>
#include <geomVertexWriter.h>
#include <geom.h>
#include <geomNode.h>
#include <geomTriangles.h>
#include <configVariableInt.h>
#include <modelRoot.h>
#include <textNode.h>
#include <pStatCollector.h>
#include <pStatTimer.h>
#include <depthOffsetAttrib.h>

static PStatCollector decal_collector( "BSP:DecalTrace" );
static PStatCollector decal_trace_collector( "BSP:DecalTrace:FindDecalPosition" );
static PStatCollector decal_node_collector( "BSP:DecalTrace:DecalNode" );
static PStatCollector decal_state_collector( "BSP:DecalTrace:DecalState" );
static PStatCollector decal_add_geom_collector( "BSP:DecalTrace:InsertGeometry" );
static PStatCollector decal_init_collector( "BSP:DecalTrace:InitDecalInfo" );

static ConfigVariableInt decals_max( "decals_max", 20 );
static ConfigVariableBool decals_remove_overlapping( "decals_remove_overlapping", true );

static const int MAX_DECALCLIPVERT = 48;
static const float DECAL_CLIP_EPSILON = 0.01f;
static const float DECAL_DISTANCE = 4.0f;
static const float SIN_45_DEGREES = 0.70710678118654752440084436210485f;

static PT( InternalName ) in_texcoord_lightmap = InternalName::get_texcoord_name( "lightmap" );

static PT( GeomVertexFormat ) decal_format_no_lightmap = nullptr;
static const GeomVertexFormat *get_decal_format_no_lightmap()
{
	if ( !decal_format_no_lightmap )
	{
		PT( GeomVertexArrayFormat ) array = new GeomVertexArrayFormat;
		array->add_column( InternalName::get_vertex(), 3, GeomEnums::NT_stdfloat, GeomEnums::C_point );
		array->add_column( InternalName::get_normal(), 3, GeomEnums::NT_stdfloat, GeomEnums::C_normal );
		array->add_column( InternalName::get_texcoord(), 2, GeomEnums::NT_stdfloat, GeomEnums::C_texcoord );
		decal_format_no_lightmap = new GeomVertexFormat;
		decal_format_no_lightmap->add_array( array );
		GeomVertexFormat::register_format( decal_format_no_lightmap );
	}

	return decal_format_no_lightmap;
}

static PT( GeomVertexFormat ) decal_format_lightmap = nullptr;
static const GeomVertexFormat *get_decal_format_lightmap()
{
	if ( !decal_format_lightmap )
	{
		PT( GeomVertexArrayFormat ) array = new GeomVertexArrayFormat;
		array->add_column( InternalName::get_vertex(), 3, GeomEnums::NT_stdfloat, GeomEnums::C_point );
		array->add_column( InternalName::get_normal(), 3, GeomEnums::NT_stdfloat, GeomEnums::C_normal );
		array->add_column( InternalName::get_texcoord(), 2, GeomEnums::NT_stdfloat, GeomEnums::C_texcoord );
		array->add_column( in_texcoord_lightmap, 2, GeomEnums::NT_stdfloat, GeomEnums::C_texcoord );
		decal_format_lightmap = new GeomVertexFormat;
		decal_format_lightmap->add_array( array );
		GeomVertexFormat::register_format( decal_format_lightmap );
	}

	return decal_format_lightmap;
}

static CPT( RenderAttrib ) depth_offset_attrib = nullptr;
static const RenderAttrib *get_depth_offset_attrib()
{
	if ( !depth_offset_attrib )
	{
		depth_offset_attrib = DepthOffsetAttrib::make( 1 );
	}
	return depth_offset_attrib;
}

static CPT( RenderAttrib ) auto_shader_attrib = nullptr;
static const RenderAttrib *get_auto_shader_attrib()
{
	if ( !auto_shader_attrib )
	{
		auto_shader_attrib = ShaderAttrib::make();
		auto_shader_attrib = DCAST( ShaderAttrib, auto_shader_attrib )->set_shader_auto( 1 );
	}
	return auto_shader_attrib;
}

static CPT( RenderAttrib ) dual_transparency_attrib = nullptr;
static const RenderAttrib *get_dual_transparency_attrib()
{
	if ( !dual_transparency_attrib )
	{
		dual_transparency_attrib = TransparencyAttrib::make( TransparencyAttrib::M_dual );
	}
	return dual_transparency_attrib;
}

static CPT( RenderAttrib ) no_transparency_attrib = nullptr;
static const RenderAttrib *get_no_transparency_attrib()
{
	if ( !no_transparency_attrib )
	{
		no_transparency_attrib = TransparencyAttrib::make( TransparencyAttrib::M_none );
	}
	return no_transparency_attrib;
}

struct decalvert_t
{
	LVector3 position;
	LVector2 coords;
};

static decalvert_t g_DecalClipVerts[MAX_DECALCLIPVERT];
static decalvert_t g_DecalClipVerts2[MAX_DECALCLIPVERT];

struct decalinfo_t
{
	decalinfo_t( const LPoint3 &pos, const LVector2 &scale, const BSPMaterial *mat, const bspdata_t *bspdata )
	{
		s_axis = nullptr;
		face = nullptr;

		data = bspdata;
		position = pos;
		decal_scale = LPoint2( 1.0f / ( scale[0] * 16 ), 1.0f / ( scale[1] * 16 ) );
		decal_size = 1.0f / decal_scale.length();
		vert_count = 0;

		material = mat;
		lightmap = false;
		bumped_lightmap = false;
		if ( mat->is_lightmapped() )
		{
			lightmap = true;
			if ( mat->has_bumpmap() )
			{
				bumped_lightmap = true;
			}
		}

		const GeomVertexFormat *format;
		if ( lightmap )
		{
			format = get_decal_format_lightmap();
		}
		else
		{
			format = get_decal_format_no_lightmap();
		}
		vdata = new GeomVertexData( "decal", format, GeomEnums::UH_static );
		vdata->reserve_num_rows( 256 );
		vdata_row_count = 0;

		geom = new Geom( vdata );
	}

	void change_surface( const dface_t *dface )
	{
		face = dface;
		VectorCopy( data->dplanes[face->planenum].normal, surface_normal );
	}

	//////////////////////////////////////////////////
	// Constant across all surfaces
	LPoint3 position;
	LVector2 decal_scale;
	float decal_size;
	const bspdata_t *data;

	//////////////////////////////////////////////////
	// Updated for each surface being decalled
	const dface_t *face;
	LVector3 surface_normal;
	LVector2 delta;
	LVector3 texture_space_basis[3];
	LVector3 *s_axis;
	
	int vert_count;

	const BSPMaterial *material;
	bool lightmap;
	bool bumped_lightmap;

	int vdata_row_count;
	PT( GeomVertexData ) vdata;
	PT( Geom ) geom;
};

// Template classes for the clipper.
class CPlane_Top
{
public:
	static inline bool Inside( decalvert_t *pVert )
	{
		return pVert->coords[1] < 1;
	}
	static inline float Clip( decalvert_t *one, decalvert_t *two )
	{
		return ( 1 - one->coords[1] ) / ( two->coords[1] - one->coords[1] );
	}
};

class CPlane_Left
{
public:
	static inline bool Inside( decalvert_t *pVert )
	{
		return pVert->coords[0] > 0;
	}
	static inline float Clip( decalvert_t *one, decalvert_t *two )
	{
		return one->coords[0] / ( one->coords[0] - two->coords[0] );
	}
};

class CPlane_Right
{
public:
	static inline bool Inside( decalvert_t *pVert )
	{
		return pVert->coords[0] < 1;
	}
	static inline float Clip( decalvert_t *one, decalvert_t *two )
	{
		return ( 1 - one->coords[0] ) / ( two->coords[0] - one->coords[0] );
	}
};

class CPlane_Bottom
{
public:
	static inline bool Inside( decalvert_t *pVert )
	{
		return pVert->coords[1] > 0;
	}
	static inline float Clip( decalvert_t *one, decalvert_t *two )
	{
		return one->coords[1] / ( one->coords[1] - two->coords[1] );
	}
};

template <class Clipper = CPlane_Top>
static inline void Intersect( Clipper &clip, decalvert_t *one, decalvert_t *two,
	decalvert_t *out )
{
	float t = Clipper::Clip( one, two );

	VectorLerp( one->position, two->position, t, out->position );
	Vector2DLerp( one->coords, two->coords, t, out->coords );
}

template <class Clipper = CPlane_Top>
static inline int SHClip( decalvert_t *pDecalClipVerts, int vertCount,
	decalvert_t *out, Clipper &clip )
{
	int j, outCount;
	decalvert_t *s, *p;

	outCount = 0;

	s = &pDecalClipVerts[vertCount - 1];
	for ( j = 0; j < vertCount; j++ )
	{
		p = &pDecalClipVerts[j];
		if ( Clipper::Inside( p ) )
		{
			if ( Clipper::Inside( s ) )
			{
				*out = *p;
				outCount++;
				out++;
			}
			else
			{
				Intersect( clip, s, p, out );
				out++;
				outCount++;

				*out = *p;
				outCount++;
				out++;
			}
		}
		else
		{
			if ( Clipper::Inside( s ) )
			{
				Intersect( clip, p, s, out );
				out++;
				outCount++;
			}
		}
		s = p;
	}

	return outCount;
}

void R_DecalComputeBasis( decalinfo_t *decal )
{
	LVector3 *texture_space_basis = decal->texture_space_basis;
	const LVector3 &surface_normal = decal->surface_normal;
	LVector3 *s_axis = decal->s_axis;

	// s, t, textureSpaceNormal (T cross S = textureSpaceNormal(N))
	//   N
	//   \   
	//    \     
	//     \  
	//      |---->S
	//      |
	//		|
	//      |T
	// S = textureSpaceBasis[0]
	// T = textureSpaceBasis[1]
	// N = textureSpaceBasis[2]

	// Get the surface normal
	texture_space_basis[2] = surface_normal;

	if ( s_axis )
	{
		// T = S cross N
		texture_space_basis[1] = s_axis->cross( texture_space_basis[2] );

		// Make sure they aren't parallel or antiparallel
		// In that case, fall back to the normal algorithm.
		if ( texture_space_basis[1].dot( texture_space_basis[1] ) > 1e-6 )
		{
			// S = N cross T
			texture_space_basis[0] = texture_space_basis[2].cross( texture_space_basis[1] );

			texture_space_basis[0].normalize();
			texture_space_basis[1].normalize();
			return;
		}

		// Fall through to the standard algorithm for parallel or antiparallel
	}

	// floor/ceiling?
	if ( fabsf( surface_normal[2] ) > SIN_45_DEGREES )
	{
		texture_space_basis[0][0] = 1.0f;
		texture_space_basis[0][1] = 0.0f;
		texture_space_basis[0][2] = 0.0f;

		// T = S cross N
		texture_space_basis[1] = texture_space_basis[0].cross( texture_space_basis[2] );
		// S = N cross T
		texture_space_basis[0] = texture_space_basis[2].cross( texture_space_basis[1] );
	}
	// wall
	else
	{
		texture_space_basis[1][0] = 0.0f;
		texture_space_basis[1][1] = 0.0f;
		texture_space_basis[1][2] = -1.0f;

		// S = N cross T
		texture_space_basis[0] = texture_space_basis[2].cross( texture_space_basis[1] );
		// T = S cross N
		texture_space_basis[1] = texture_space_basis[0].cross( texture_space_basis[2] );
	}

	texture_space_basis[0].normalize();
	texture_space_basis[1].normalize();
}

void R_SetupDecalTextureSpaceBasis( decalinfo_t *decal )
{
	// Compute the non-scaled decal basis
	R_DecalComputeBasis( decal );
	decal->texture_space_basis[0] *= decal->decal_scale[0];
	decal->texture_space_basis[1] *= decal->decal_scale[1];
}

void R_SetupDecalClip( decalinfo_t *decal )
{
	R_SetupDecalTextureSpaceBasis( decal );
	decal->delta[0] = decal->position.dot( decal->texture_space_basis[0] );
	decal->delta[1] = decal->position.dot( decal->texture_space_basis[1] );
}

void R_AddDecalVert( decalinfo_t *decal, const dvertex_t *vert, int idx )
{
	VectorCopy( vert->point, g_DecalClipVerts[idx].position );
	g_DecalClipVerts[idx].coords[0] = g_DecalClipVerts[idx].position.dot( decal->texture_space_basis[0] ) - decal->delta[0] + 0.5f;
	g_DecalClipVerts[idx].coords[1] = g_DecalClipVerts[idx].position.dot( decal->texture_space_basis[1] ) - decal->delta[1] + 0.5f;
}

void R_SetupDecalVertsForSurface( decalinfo_t *decal )
{
	decal->vert_count = 0;

	for ( int nedge = 0; nedge < decal->face->numedges; nedge++ )
	{
		const int surfedge = decal->data->dsurfedges[decal->face->firstedge + nedge];
		const dedge_t *edge;
		int index;
		if ( surfedge >= 0 )
		{
			edge = &decal->data->dedges[surfedge];
			index = 0;
		}
		else
		{
			edge = &decal->data->dedges[-surfedge];
			index = 1;
		}

		R_AddDecalVert( decal, decal->data->dvertexes + edge->v[index], decal->vert_count++ );
	}
}

void R_DoDecalSHClip( decalinfo_t *info )
{
	CPlane_Top top;
	CPlane_Left left;
	CPlane_Right right;
	CPlane_Bottom bottom;
	
	// Clip the polygon to the decal texture space
	int out_count = SHClip( g_DecalClipVerts, info->vert_count, g_DecalClipVerts2, top );
	out_count = SHClip( g_DecalClipVerts2, out_count, g_DecalClipVerts, left );
	out_count = SHClip( g_DecalClipVerts, out_count, g_DecalClipVerts2, right );
	out_count = SHClip( g_DecalClipVerts2, out_count, g_DecalClipVerts, bottom );
	info->vert_count = out_count;
}

void R_DecalSurface( const dface_t *face, decalinfo_t *pinfo )
{
	BSPLoader *loader = BSPLoader::get_global_ptr();
	int facenum = face - pinfo->data->dfaces;

	pinfo->change_surface( face );

	R_SetupDecalClip( pinfo );
	R_SetupDecalVertsForSurface( pinfo );
	R_DoDecalSHClip( pinfo );

	if ( pinfo->vert_count == 0 )
		return;

	////////////////////////////////////////////////////////////////////////////////////
	// Generate the decal geometry

	int first_row = pinfo->vdata_row_count;
	GeomVertexWriter vtx_writer( pinfo->vdata, InternalName::get_vertex() );
	vtx_writer.set_row( first_row );
	GeomVertexWriter norm_writer( pinfo->vdata, InternalName::get_normal() );
	norm_writer.set_row( first_row );
	GeomVertexWriter uv_writer( pinfo->vdata, InternalName::get_texcoord() );
	uv_writer.set_row( first_row );
	GeomVertexWriter lm_uv_writer;
	if ( pinfo->lightmap )
	{
		lm_uv_writer = GeomVertexWriter( pinfo->vdata, in_texcoord_lightmap );
		lm_uv_writer.set_row( first_row );
	}

	for ( int i = pinfo->vert_count - 1; i >= 0; i-- )
	{
		decalvert_t *cvert = g_DecalClipVerts + i;

		vtx_writer.add_data3f( cvert->position / 16.0f );
		norm_writer.add_data3f( pinfo->surface_normal );
		uv_writer.add_data2f( cvert->coords );
		if ( pinfo->lightmap )
		{
			lm_uv_writer.add_data2f( loader->get_lightcoords( facenum, cvert->position ) );
		}

		pinfo->vdata_row_count++;
	}

	PT( GeomTriangles ) tris = new GeomTriangles( GeomEnums::UH_static );
	int ntris = pinfo->vert_count - 2;
	for ( int tri = 0; tri < ntris; tri++ )
	{
		tris->add_vertices(
			first_row,
			first_row + ( ( tri + 1 ) % pinfo->vert_count ),
			first_row + ( ( tri + 2 ) % pinfo->vert_count )
		);
	}
	tris->close_primitive();

	pinfo->geom->add_primitive( tris );
}

void R_DecalNodeSurfaces( const dnode_t *pnode, decalinfo_t *info )
{
	for ( int i = 0; i < pnode->numfaces; i++ )
	{
		R_DecalSurface( info->data->dfaces + ( pnode->firstface + i ), info );
	}
}

void R_DecalLeaf( int leafnum, decalinfo_t *info )
{
	const dleaf_t *leaf = info->data->dleafs + leafnum;
	for ( int i = 0; i < leaf->nummarksurfaces; i++ )
	{
		const dface_t *face = info->data->dfaces + info->data->dmarksurfaces[leaf->firstmarksurface + i];
		const dplane_t *plane = info->data->dplanes + face->planenum;
		float dist = fabsf( DotProduct( info->position, plane->normal ) - plane->dist );
		if ( dist < DECAL_DISTANCE )
		{
			R_DecalSurface( face, info );
		}
	}
}

void R_DecalNode( int nodenum, decalinfo_t *info )
{
	const dplane_t *splitplane;
	float dist;

	if ( nodenum < 0 )
	{
		R_DecalLeaf( ~nodenum, info );
		return;
	}

	const dnode_t *pnode = info->data->dnodes + nodenum;
	splitplane = info->data->dplanes + pnode->planenum;
	dist = DotProduct( info->position, splitplane->normal ) - splitplane->dist;
	if ( dist > info->decal_size )
	{
		R_DecalNode( pnode->children[0], info );
	}
	else if ( dist < -info->decal_size )
	{
		R_DecalNode( pnode->children[1], info );
	}
	else
	{
		if ( dist < DECAL_DISTANCE && dist > -DECAL_DISTANCE )
			R_DecalNodeSurfaces( pnode, info );
		R_DecalNode( pnode->children[0], info );
		R_DecalNode( pnode->children[1], info );
	}
}

/**
 * Trace a decal onto the world.
 */
NodePath DecalManager::decal_trace( const std::string &decal_material, const LPoint2 &decal_scale,
        float rotate, const LPoint3 &start, const LPoint3 &end )
{
	PStatTimer timer( decal_collector );

	///////////////////////////////////////////////////////////////////////////////////////
        // Find the surface to decal
	LVector3 decal_origin;
	{
		PStatTimer trace_timer( decal_trace_collector );

		RayTraceHitResult tr = _loader->get_trace()->get_scene()->trace_line( start * 16, end * 16, TRACETYPE_WORLD );
		// Do we have a surface to decal?
		if ( !tr.has_hit() )
			return NodePath();

		VectorLerp( start, end, tr.get_hit_fraction(), decal_origin );
		decal_origin *= 16.0f;
	}
	

	///////////////////////////////////////////////////////////////////////////////////////

	const BSPMaterial *mat = BSPMaterial::get_from_file( decal_material );

	decal_init_collector.start();
	decalinfo_t info(
		decal_origin,
		decal_scale,
		mat,
		_loader->get_bspdata() );
	decal_init_collector.stop();
	
	decal_node_collector.start();
	R_DecalNode( 0, &info );
	decal_node_collector.stop();

	///////////////////////////////////////////////////////////////////////////////////////
	// Setup decal render state / geometry

	decal_state_collector.start();

	// Set the desired material
	CPT( RenderAttrib ) bma = BSPMaterialAttrib::make( mat );

	const RenderAttrib *transparency;
	// Enable transparency
	if ( mat->has_transparency() )
	{
		transparency = get_dual_transparency_attrib();
	}
	else
	{
		transparency = get_no_transparency_attrib();
	}

	CPT( RenderState ) decal_state = RenderState::make( get_depth_offset_attrib(), 
		get_auto_shader_attrib(), bma, transparency, 1 );

	// Bind lightmaps if needed
	if ( info.lightmap )
	{
		LightmapPaletteDirectory::LightmapPaletteEntry *entry = _loader->get_lightmap_dir()->entries[0];
		Texture *lm_tex = entry->palette_tex;

		CPT( RenderAttrib ) lightmap_tex_attr = TextureAttrib::make();
		if ( info.bumped_lightmap )
		{
			lightmap_tex_attr = DCAST( TextureAttrib, lightmap_tex_attr )->
				add_on_stage( TextureStages::get_bumped_lightmap(), lm_tex );
		}
		else
		{
			lightmap_tex_attr = DCAST( TextureAttrib, lightmap_tex_attr )->
				add_on_stage( TextureStages::get_lightmap(), lm_tex );
		}

		decal_state = decal_state->set_attrib( lightmap_tex_attr );
	}

	decal_state_collector.stop();

	decal_add_geom_collector.start();
	PT( GeomNode ) decal_geomnode = new GeomNode( "decal" );
	decal_geomnode->add_geom( info.geom, decal_state );
	NodePath decalnp = _loader->get_result().attach_new_node( decal_geomnode );
	decal_add_geom_collector.stop();

	///////////////////////////////////////////////////////////////////////////////////////

        LPoint3 mins, maxs;
        //decalnp.calc_tight_bounds( mins, maxs );
        PT( BoundingBox ) bounds = new BoundingBox( mins, maxs );

        if ( decals_remove_overlapping.get_value() )
        {
                for ( int i = (int)_decals.size() - 1; i >= 0; i-- )
                {
                        Decal *other = _decals[i];

                        // Check if we overlap with this decal.
                        //
                        // Only remove this decal if it is smaller than the decal
                        // we are wanting to create over it, and it is not a static
                        // decal (placed by the level designer, etc).
                        if ( other->bounds->contains( bounds ) != BoundingVolume::IF_no_intersection &&
                                other->bounds->get_volume() <= bounds->get_volume() &&
                                ( other->flags & DECALFLAGS_STATIC ) == 0 )
                        {
                                if ( !other->decalnp.is_empty() )
                                        other->decalnp.remove_node();
                                _decals.erase( std::find( _decals.begin(), _decals.end(), other ) );
                        }
                }
        }

        if ( _decals.size() == decals_max.get_value() )
        {
                // Remove the oldest decal to make space for the new one.
                Decal *d = _decals.back();
                if ( !d->decalnp.is_empty() )
                        d->decalnp.remove_node();
                _decals.pop_back();
        }

        PT( Decal ) decal = new Decal;
        decal->decalnp = decalnp;
        decal->bounds = bounds;
        decal->flags = DECALFLAGS_NONE;
        _decals.push_front( decal );
        
        // Decals should not cast shadows
        decalnp.hide( CAMERA_SHADOW );

        return decalnp;
}

void DecalManager::cleanup()
{
        for ( size_t i = 0; i < _decals.size(); i++ )
        {
                Decal *d = _decals[i];
                if ( !d->decalnp.is_empty() )
                        d->decalnp.remove_node();
        }
        _decals.clear();
}
