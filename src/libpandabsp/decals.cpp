/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file decals.cpp
 * @author Brian Lach
 * @date March 08, 2019
 *
 * @desc Decals on the world!
 *
 */

#include "decals.h"
#include "bsp_trace.h"
#include "bsploader.h"
#include "bsp_material.h"
#include "shader_generator.h"
#include "bsp_render.h"
#include "mathlib.h"

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
#include <colorBlendAttrib.h>
#include <alphaTestAttrib.h>
#include <depthWriteAttrib.h>
#include <colorWriteAttrib.h>
#include <cullFaceAttrib.h>
#include <rigidBodyCombiner.h>
#include <bulletWorld.h>
#include <bulletClosestHitRayResult.h>
#include <bitMask.h>
#include <geomTristrips.h>

static const BitMask32 world_bitmask = BitMask32::bit( 1 ) | BitMask32::bit( 2 );

static PStatCollector decal_collector( "BSP:DecalTrace" );
static PStatCollector decal_trace_collector( "BSP:DecalTrace:FindDecalPosition" );
static PStatCollector decal_node_collector( "BSP:DecalTrace:DecalNode" );
static PStatCollector decal_state_collector( "BSP:DecalTrace:DecalState" );
static PStatCollector decal_add_geom_collector( "BSP:DecalTrace:InsertGeometry" );
static PStatCollector decal_init_collector( "BSP:DecalTrace:InitDecalInfo" );
static PStatCollector decal_collect( "BSP:DecalTrace:BatchDecals" );

static ConfigVariableInt decals_max( "decals_max", 20 );
static ConfigVariableBool decals_remove_overlapping( "decals_remove_overlapping", true );

static const int MAX_DECALCLIPVERT = 48;
static const float DECAL_CLIP_EPSILON = 0.01f;
static const float DECAL_DISTANCE = 4.0f;
static const float SIN_45_DEGREES = 0.70710678118654752440084436210485f;

static PT( InternalName ) in_texcoord_lightmap = InternalName::get_texcoord_name( "lightmap" );

static const GeomVertexFormat *get_decal_format_no_lightmap()
{
	return GeomVertexFormat::get_v3n3c4t2();
}

static PT( GeomVertexFormat ) decal_format_lightmap = nullptr;
static const GeomVertexFormat *get_decal_format_lightmap()
{
	if ( !decal_format_lightmap )
	{
		PT( GeomVertexArrayFormat ) array = new GeomVertexArrayFormat;
		array->add_column( InternalName::get_vertex(), 3, GeomEnums::NT_stdfloat, GeomEnums::C_point );
		array->add_column( InternalName::get_normal(), 3, GeomEnums::NT_stdfloat, GeomEnums::C_normal );
		array->add_column( InternalName::get_color(), 4, GeomEnums::NT_stdfloat, GeomEnums::C_color );
		array->add_column( InternalName::get_texcoord(), 2, GeomEnums::NT_stdfloat, GeomEnums::C_texcoord );
		array->add_column( in_texcoord_lightmap, 2, GeomEnums::NT_stdfloat, GeomEnums::C_texcoord );
		decal_format_lightmap = new GeomVertexFormat;
		decal_format_lightmap->add_array( array );
		GeomVertexFormat::register_format( decal_format_lightmap );
	}

	return decal_format_lightmap;
}

#define GET_ATTRIB(varname) get_ ##varname ## _attrib()
#define DEFINE_ATTRIB(varname, decl) \
static CPT(RenderAttrib) varname ## _attrib = nullptr; \
static const RenderAttrib *get_ ##varname ## _attrib() \
{ \
	if (!varname ## _attrib) \
	{ \
		varname ## _attrib = decl;\
	} \
	return varname ## _attrib;\
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

DEFINE_ATTRIB( depth_offset, DepthOffsetAttrib::make( 1 ) )
DEFINE_ATTRIB( transparency, TransparencyAttrib::make( TransparencyAttrib::M_alpha ) )
DEFINE_ATTRIB( no_transparency, TransparencyAttrib::make( TransparencyAttrib::M_none ) )
DEFINE_ATTRIB( decal_modulate, ColorBlendAttrib::make( ColorBlendAttrib::M_add,
						       ColorBlendAttrib::O_fbuffer_color, ColorBlendAttrib::O_incoming_color) )
DEFINE_ATTRIB( decal_alpha, AlphaTestAttrib::make( AlphaTestAttrib::M_greater, 0.0f ) )
DEFINE_ATTRIB( depth_write_off, DepthWriteAttrib::make( DepthWriteAttrib::M_off ) )
DEFINE_ATTRIB( no_alpha_write, ColorWriteAttrib::make( ColorWriteAttrib::C_rgb ) )
DEFINE_ATTRIB( vertex_color, ColorAttrib::make_vertex() )
DEFINE_ATTRIB( double_side, CullFaceAttrib::make( CullFaceAttrib::M_cull_none ) )

struct decalvert_t
{
	LVector3 position;
	LVector2 coords;
};

static decalvert_t g_DecalClipVerts[MAX_DECALCLIPVERT];
static decalvert_t g_DecalClipVerts2[MAX_DECALCLIPVERT];

struct decalinfo_t
{
	decalinfo_t( const LPoint3 &pos, const LVector2 &scale, const LColorf &color, const BSPMaterial *mat, const bspdata_t *bspdata )
	{
		s_axis = nullptr;
		face = nullptr;

		data = bspdata;
		position = pos;
		decal_scale = LPoint2( 1.0f / ( scale[0] * 16 ), 1.0f / ( scale[1] * 16 ) );
		decal_size = 1.0f / decal_scale.length();
		decal_color = color;
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
	LColor decal_color;
	LMatrix4f decal_world_to_model;

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
	GeomVertexWriter col_writer( pinfo->vdata, InternalName::get_color() );
	col_writer.set_row( first_row );
	GeomVertexWriter lm_uv_writer;
	if ( pinfo->lightmap )
	{
		lm_uv_writer = GeomVertexWriter( pinfo->vdata, in_texcoord_lightmap );
		lm_uv_writer.set_row( first_row );
	}

	LVector3 local_normal = pinfo->decal_world_to_model.xform_vec( pinfo->surface_normal );

	for ( int i = pinfo->vert_count - 1; i >= 0; i-- )
	{
		decalvert_t *cvert = g_DecalClipVerts + i;

		LPoint3 local_pos = pinfo->decal_world_to_model.xform_point( cvert->position / 16.0f );
		vtx_writer.add_data3f( local_pos );
		norm_writer.add_data3f( local_normal );
		uv_writer.add_data2f( cvert->coords );
		col_writer.add_data4f( pinfo->decal_color );
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
void DecalManager::decal_trace( const std::string &decal_material, const LPoint2 &decal_scale,
				float rotate, const LPoint3 &start, const LPoint3 &end, const LColorf &decal_color,
				const int flags )
{
	PStatTimer timer( decal_collector );

	///////////////////////////////////////////////////////////////////////////////////////
        // Find the surface to decal
	LVector3 decal_origin;
	int headnode = 0;
	int modelnum = 0;
	int merged_modelnum = 0;
	brush_model_data_t mdata;
	bool is_studio = false;
	NodePath hitbox;
	{
		PStatTimer trace_timer( decal_trace_collector );

		BulletClosestHitRayResult result = _loader->get_physics_world()->
			ray_test_closest( start, end, world_bitmask );

		if ( !result.has_hit() )
			return;

		int triangle_idx = result.get_triangle_index();
		hitbox = NodePath( result.get_node() );
		BulletRigidBodyNode *node = DCAST( BulletRigidBodyNode, result.get_node() );
		int temp_modelnum = _loader->get_brush_triangle_model_fast( node, triangle_idx );
		if ( temp_modelnum != -1 )
		{
			modelnum = temp_modelnum;
			mdata = _loader->get_brush_model_data( modelnum );
			merged_modelnum = mdata.merged_modelnum;
			const dmodel_t *model = _loader->get_bspdata()->dmodels + modelnum;
			headnode = model->headnode[0];
		}
		else
		{
			return;
		}

		VectorLerp( start, end, result.get_hit_fraction(), decal_origin );
		
		if ( merged_modelnum != 0 && !is_studio )
		{
			// A non-world model can be moved around.
			// In order to correctly decal, we must move the decal position back
			// relative to the model's original transform.
			CPT( TransformState ) ts = mdata.model_root.get_net_transform();

			LPoint3 delta_origin = ts->get_pos() - mdata.origin;
			LQuaternion delta_quat = ts->get_norm_quat();
			LVector3 delta_scale = ts->get_scale();

			CPT( TransformState ) delta_ts = TransformState::make_pos_quat_scale( delta_origin, delta_quat, delta_scale );
			LMatrix4 matrix = delta_ts->get_mat();
			matrix.invert_in_place();

			decal_origin = matrix.xform_point( decal_origin );
		}

		decal_origin *= 16.0f;
	}
	

	///////////////////////////////////////////////////////////////////////////////////////

	const BSPMaterial *mat = BSPMaterial::get_from_file( decal_material );

	decal_init_collector.start();
	decalinfo_t info(
		decal_origin,
		decal_scale,
		decal_color,
		mat,
		_loader->get_bspdata() );
	if ( !is_studio )
	{
		if ( merged_modelnum != 0 )
		{
			info.decal_world_to_model = mdata.origin_matrix;
			info.decal_world_to_model.invert_in_place();
		}
		else
		{
			info.decal_world_to_model = LMatrix4f::ident_mat();
		}
		decal_init_collector.stop();

		decal_node_collector.start();
		R_DecalNode( headnode, &info );
		decal_node_collector.stop();
	}
	else
	{
		NodePath studio_root = hitbox.get_parent();
		NodePathCollection geom_nodes = studio_root.find_all_matches( "**/+GeomNode" );
		for ( int i = 0; i < geom_nodes.size(); i++ )
		{
			NodePath geom_np = geom_nodes[i];
			GeomNode *gn = DCAST( GeomNode, geom_np.node() );
			for ( int j = 0; j < gn->get_num_geoms(); j++ )
			{
				const Geom *geom = gn->get_geom( j );
				const GeomVertexData *vdata = geom->get_vertex_data();
				for ( int k = 0; k < geom->get_num_primitives(); k++ )
				{
					const GeomPrimitive *prim = geom->get_primitive( k );
					for ( int l = 0; l < prim->get_num_primitives(); l++ )
					{
						int start = prim->get_primitive_start( l );
						int end = prim->get_primitive_end( l );
						if ( prim->is_of_type( GeomTristrips::get_class_type() ) )
						{
							for ( int vidx = start; vidx < end - 2; vidx++ )
							{
								bool ccw = ( vidx & 0x1 ) == 0;
								int ti1 = vidx;
								int ti2 = vidx + 1 + ccw;
								int ti3 = vidx + 2 - ccw;
								

							}
						}
					}
				}
			}
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////
	// Setup decal render state / geometry

	decal_state_collector.start();

	// Set the desired material
	CPT( RenderAttrib ) bma = BSPMaterialAttrib::make( mat );

	bool is_decal_modulate = mat->get_shader() == "DecalModulate";

	const RenderAttrib *transparency;
	// Enable transparency
	if ( mat->has_transparency() )
	{
		transparency = GET_ATTRIB( transparency );
	}
	else
	{
		transparency = GET_ATTRIB( no_transparency );
	}

	const RenderAttrib *attribs[] = {
		GET_ATTRIB( depth_offset ),
		GET_ATTRIB( auto_shader ),
		bma,
		transparency,
		GET_ATTRIB( depth_write_off ),
		GET_ATTRIB( no_alpha_write ), // don't write to dest alpha
		GET_ATTRIB( decal_alpha ),
		GET_ATTRIB( vertex_color )
	};

	CPT( RenderState ) decal_state = RenderState::make( attribs, ARRAYSIZE( attribs ), 1 );

	// Bind lightmaps if needed
	if ( info.lightmap )
	{
		LightmapPaletteDirectory::LightmapPaletteEntry *entry = _loader->get_lightmap_dir()->entries[0];
		Texture *lm_tex = entry->palette_tex;

		CPT( RenderAttrib ) lightmap_tex_attr = TextureAttrib::make();
		//if ( info.bumped_lightmap )
		//{
		//	lightmap_tex_attr = DCAST( TextureAttrib, lightmap_tex_attr )->
		//		add_on_stage( TextureStages::get_bumped_lightmap(), lm_tex );
		//}
		//else
		//{
			lightmap_tex_attr = DCAST( TextureAttrib, lightmap_tex_attr )->
				add_on_stage( TextureStages::get_lightmap(), lm_tex );
		//}

		decal_state = decal_state->set_attrib( lightmap_tex_attr );
	}

	// Yikes
	if ( is_decal_modulate )
	{
		decal_state = decal_state->set_attrib( GET_ATTRIB( decal_modulate ) );
	}

	decal_state_collector.stop();

	decal_add_geom_collector.start();
	PT( GeomNode ) decal_geomnode = new GeomNode( "decal" );
	decal_geomnode->add_geom( info.geom, decal_state );
	NodePath decalnp = NodePath( decal_geomnode );
	NodePath rbcnp = NodePath( mdata.decal_rbc );
	decalnp.reparent_to( rbcnp );
	decal_add_geom_collector.stop();

	///////////////////////////////////////////////////////////////////////////////////////

        LPoint3 mins, maxs;
        //decalnp.calc_tight_bounds( mins, maxs );
        PT( BoundingBox ) bounds = new BoundingBox( mins, maxs );

	if ( decals_remove_overlapping.get_value() && ( flags & DECALFLAGS_STATIC ) == 0 )
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
				if ( other->brush_modelnum != merged_modelnum )
					_loader->get_brush_model_data( other->brush_modelnum ).decal_rbc->collect();
                                _decals.erase( std::find( _decals.begin(), _decals.end(), other ) );
                        }
                }
        }

        if ( _decals.size() == decals_max.get_value() )
        {
                // Remove the oldest decal to make space for the new one.
                Decal *d = _decals.back();
		if ( !d->decalnp.is_empty() )
		{
			d->decalnp.remove_node();
			if ( d->brush_modelnum != merged_modelnum )
				_loader->get_brush_model_data( d->brush_modelnum ).decal_rbc->collect();
		}
                _decals.pop_back();
        }

        PT( Decal ) decal = new Decal;
        decal->decalnp = decalnp;
        decal->bounds = bounds;
        decal->flags = flags;
	decal->brush_modelnum = merged_modelnum;
	if ( ( flags & DECALFLAGS_STATIC ) != 0 )
		_map_decals.push_back( decal );
	else
		_decals.push_front( decal );

	decal_collect.start();
	mdata.decal_rbc->collect();
	//_decal_rbc->collect();
	decal_collect.stop();
	//_decal_rbc->get_internal_scene().ls();
}

void DecalManager::studio_decal_trace( const std::string &decal_material, const LPoint2 &decal_scale,
				       float rotate, const LPoint3 &start, const LPoint3 &end,
				       const LColorf &decal_color, const int flags )
{

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

	for ( size_t i = 0; i < _map_decals.size(); i++ )
	{
		Decal *d = _map_decals[i];
		if ( !d->decalnp.is_empty() )
			d->decalnp.remove_node();
	}
	_map_decals.clear();

	if ( !_decal_root.is_empty() )
		_decal_root.remove_node();
	_decal_rbc = nullptr;
}

void DecalManager::init()
{
	_decal_rbc = new RigidBodyCombiner( "decal-root" );
	_decal_root = NodePath( _decal_rbc );
	_decal_root.reparent_to( _loader->get_result() );
	_decal_root.hide( CAMERA_SHADOW );
}

DecalManager::DecalManager( BSPLoader *loader ) :
	_loader( loader )
{
}
