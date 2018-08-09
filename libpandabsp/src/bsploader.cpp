#include "bsploader.h"

#include "bsp_render.h"
#include "bspfile.h"
#include "entity.h"
#include "mathlib.h"

#include <array>
#include <bitset>
#include <math.h>

#include <asyncTaskManager.h>
#include <eggData.h>
#include <eggPolygon.h>
#include <eggVertexUV.h>
#include <eggWriter.h>
#include <geomNode.h>
#include <load_egg_file.h>
#include <loader.h>
#include <nodePathCollection.h>
#include <pnmFileTypeJPG.h>
#include <pointLight.h>
#include <randomizer.h>
#include <rigidBodyCombiner.h>
#include <texture.h>
#include <texturePool.h>
#include <textureStage.h>
#include <virtualFileSystem.h>
#include <directionalLight.h>
#include <ambientLight.h>
#include <spotlight.h>
#include <fog.h>
#include <lineSegs.h>
#include <nodePath_ext.h>
#include <graphicsEngine.h>
#include <boundingBox.h>
#include <pStatCollector.h>
#include <cullTraverser.h>
#include <cullTraverserData.h>
#include <cullableObject.h>
#include <transformState.h>
#include <cullHandler.h>
#include <modelRoot.h>
#include <lightReMutexHolder.h>
#include <geomVertexData.h>
#include <geomVertexRewriter.h>
#include <sceneGraphReducer.h>
#include <characterJointEffect.h>

INLINE static void flatten_node( const NodePath &node )
{
        // Mimic a flatten strong operation, but do not attempt to combine
        // Geoms, as each face needs to be it's own Geom for effective leaf
        // culling.

        SceneGraphReducer gr;
        gr.apply_attribs( node.node() );
        gr.flatten( node.node(), ~0 );
}

PStatCollector bfa_collector( "BSP:BSPFaceAttrib" );

TypeHandle BSPFaceAttrib::_type_handle;
int BSPFaceAttrib::_attrib_slot;

INLINE BSPFaceAttrib::BSPFaceAttrib() :
        RenderAttrib()
{
}

bool BSPFaceAttrib::has_cull_callback() const
{
        return false;
}

INLINE string BSPFaceAttrib::get_material() const
{
        return _material;
}

CPT( RenderAttrib ) BSPFaceAttrib::make( const string &face_material )
{
        BSPFaceAttrib *attrib = new BSPFaceAttrib;
        attrib->_material = face_material;
        return return_new( attrib );
}

CPT( RenderAttrib ) BSPFaceAttrib::make_default()
{
        BSPFaceAttrib *attrib = new BSPFaceAttrib;
        attrib->_material = "default";
        return return_new( attrib );
}

int BSPFaceAttrib::compare_to_impl( const RenderAttrib *other ) const
{
        const BSPFaceAttrib *ta = (const BSPFaceAttrib *)other;

        return _material.compare( ta->_material );
}

size_t BSPFaceAttrib::get_hash_impl() const
{
        size_t hash = 0;
        hash = string_hash::add_hash( hash, _material );
        return hash;
}

//#define VISUALIZE_PLANE_COLORS
//#define EXTRACT_LIGHTMAPS

#define DEFAULT_GAMMA 2.2
#define QRAD_GAMMA 1.8
#define DEFAULT_OVERBRIGHT 1
#define ATTN_FACTOR 0.03

// Due to some imprecision, we will expand the leaf AABBs just a tiny bit
// as a compensation. 1.0 seems like a lot, but it is defined in Hammer space
// where 1 Hammer unit is 0.0625 Panda units.
#define LEAF_NUDGE 1.0

PT( GeomNode ) UTIL_make_cube_outline( const LPoint3 &min, const LPoint3 &max,
                                       const LColor &color, PN_stdfloat thickness )
{
        LineSegs lines;
        lines.set_color( color );
        lines.set_thickness( thickness );
        lines.move_to( min );
        lines.draw_to( LVector3( min.get_x(), min.get_y(), max.get_z() ) );
        lines.draw_to( LVector3( min.get_x(), max.get_y(), max.get_z() ) );
        lines.draw_to( LVector3( min.get_x(), max.get_y(), min.get_z() ) );
        lines.draw_to( min );
        lines.draw_to( LVector3( max.get_x(), min.get_y(), min.get_z() ) );
        lines.draw_to( LVector3( max.get_x(), min.get_y(), max.get_z() ) );
        lines.draw_to( LVector3( min.get_x(), min.get_y(), max.get_z() ) );
        lines.move_to( LVector3( max.get_x(), min.get_y(), max.get_z() ) );
        lines.draw_to( max );
        lines.draw_to( LVector3( min.get_x(), max.get_y(), max.get_z() ) );
        lines.move_to( max );
        lines.draw_to( LVector3( max.get_x(), max.get_y(), min.get_z() ) );
        lines.draw_to( LVector3( min.get_x(), max.get_y(), min.get_z() ) );
        lines.move_to( LVector3( max.get_x(), max.get_y(), min.get_z() ) );
        lines.draw_to( LVector3( max.get_x(), min.get_y(), min.get_z() ) );
        return lines.create();
}

void GetParamsFromEnt( entity_t *mapent )
{
}

NotifyCategoryDef( bspfile, "" );

BSPLoader *BSPLoader::_global_ptr = nullptr;

int BSPLoader::find_leaf( const NodePath &np )
{
        return find_leaf( np.get_pos( _result ) );
}

int BSPLoader::find_leaf( const LPoint3 &pos )
{
        int i = 0;

        // Walk the BSP tree to find the index of the leaf which contains the specified
        // position.
        while ( i >= 0 )
        {
                const dnode_t *node = &g_dnodes[i];
                const dplane_t *plane = &g_dplanes[node->planenum];
                float distance = ( plane->normal[0] * pos.get_x() ) +
                        ( plane->normal[1] * pos.get_y() ) +
                        ( plane->normal[2] * pos.get_z() ) - ( plane->dist / PANDA_TO_HAMMER );

                if ( distance >= 0.0 )
                {
                        i = node->children[0];
                }
                else
                {
                        i = node->children[1];
                }

        }

        return ~i;
}

LTexCoord BSPLoader::get_vertex_uv( texinfo_t *texinfo, dvertex_t *vert, bool lightmap ) const
{
        float *vpos = vert->point;
        LVector3 vert_pos( vpos[0], vpos[1], vpos[2] );

        LVector3 s_vec, t_vec;
        float s_dist, t_dist;
        if ( !lightmap )
        {
                s_vec = LVector3( texinfo->vecs[0][0], texinfo->vecs[0][1], texinfo->vecs[0][2] );
                s_dist = texinfo->vecs[0][3];

                t_vec = LVector3( texinfo->vecs[1][0], texinfo->vecs[1][1], texinfo->vecs[1][2] );
                t_dist = texinfo->vecs[1][3];
        }
        else
        {
                s_vec = LVector3( texinfo->lightmap_vecs[0][0], texinfo->lightmap_vecs[0][1], texinfo->lightmap_vecs[0][2] );
                s_dist = texinfo->lightmap_vecs[0][3];

                t_vec = LVector3( texinfo->lightmap_vecs[1][0], texinfo->lightmap_vecs[1][1], texinfo->lightmap_vecs[1][2] );
                t_dist = texinfo->lightmap_vecs[1][3];
        }
        

        return LTexCoord( s_vec.dot( vert_pos ) + s_dist, t_vec.dot( vert_pos ) + t_dist );
}

PT( EggVertex ) BSPLoader::make_vertex( EggVertexPool *vpool, EggPolygon *poly,
                                        dedge_t *edge, texinfo_t *texinfo,
                                        dface_t *face, int k, FaceLightmapData *ld,
                                        Texture *tex )
{
        dvertex_t *vert = &g_dvertexes[edge->v[k]];
        float *vpos = vert->point;
        PT( EggVertex ) v = new EggVertex;
        v->set_pos( LPoint3d( vpos[0], vpos[1], vpos[2] ) );

        // The widths and heights are retrieved from the actual loaded textures that were referenced.
        double df_width = 1.0;
        double df_height = 1.0;
        if ( tex != nullptr )
        {
                df_width = tex->get_orig_file_x_size();
                df_height = tex->get_orig_file_y_size();
        }

        // Texture and lightmap coordinates
        LTexCoord uv = get_vertex_uv( texinfo, vert );
        LTexCoord luv = get_vertex_uv( texinfo, vert, true );
        LTexCoordd df_uv( uv.get_x() / df_width, -uv.get_y() / df_height );
        LTexCoordd lm_uv( 0, 0 );

        if ( ld->faceentry != nullptr )
        {
                // This face has an entry in a lightmap palette.
                // Transform the UVs.

                double midtexs[2];
                midtexs[0] = ld->faceentry->flipped ? ld->midtexs[1] : ld->midtexs[0];
                midtexs[1] = ld->faceentry->flipped ? ld->midtexs[0] : ld->midtexs[1];
                double midpolys[2];
                midpolys[0] = ld->faceentry->flipped ? ld->midpolys[1] : ld->midpolys[0];
                midpolys[1] = ld->faceentry->flipped ? ld->midpolys[0] : ld->midpolys[1];
                double dluv[2];
                dluv[0] = ld->faceentry->flipped ? luv[1] : luv[0];
                dluv[1] = ld->faceentry->flipped ? luv[0] : luv[1];
                int texsize[2];
                texsize[0] = ld->faceentry->flipped ? face->lightmap_size[1] : face->lightmap_size[0];
                texsize[1] = ld->faceentry->flipped ? face->lightmap_size[0] : face->lightmap_size[1];

                lm_uv.set( ( midtexs[0] + ld->faceentry->xshift + ( dluv[0] - midpolys[0] ) ),
                          ( midtexs[1] + ld->faceentry->yshift + ( dluv[1] - midpolys[1] ) ) );

                lm_uv[0] /= ld->faceentry->palette_size[0];
                lm_uv[1] /= -ld->faceentry->palette_size[1];
        }

        v->set_uv( "diffuse", df_uv );
        v->set_uv( "lightmap", lm_uv );

        return v;
}

PT( Texture ) BSPLoader::try_load_texref( texref_t *tref )
{
        if ( _texref_textures.find( tref ) != _texref_textures.end() )
        {
                return _texref_textures[tref];
        }

        string name = tref->name;

	PT( Texture ) tex = nullptr;

	size_t ext_idx = name.find_last_of( "." );
	string basename = name.substr( 0, ext_idx );
	string alpha_file = basename + "_a.rgb";
        Filename alpha_filename = Filename::from_os_specific( alpha_file );
	if ( VirtualFileSystem::get_global_ptr()->exists( alpha_filename ) )
	{
		// A corresponding alpha file exists for this texture, load that up as well.
                bspfile_cat.info()
                        << "Found corresponding alpha file " << alpha_filename.get_fullpath()
                        << " for texture " << name << "\n";
		tex = TexturePool::load_texture( name, alpha_filename );
	}
	else
	{
		// Just load the texture.
		tex = TexturePool::load_texture( name );
	}

        if ( tex != nullptr )
        {
                bspfile_cat.info()
                        << "Loaded texref " << tref->name << "\n";
                _texref_textures[tref] = tex;
                return tex;
        }

        return nullptr;
}

void BSPLoader::make_faces()
{
        bspfile_cat.info()
                << "Making faces...\n";

        // In BSP files, models are brushes that have been grouped together to be used as an entity.
        // We can group all of the face GeomNodes of the model to a root node.
        for ( int modelnum = 0; modelnum < g_nummodels; modelnum++ )
        {
                dmodel_t *model = &g_dmodels[modelnum];
                int firstface = model->firstface;
                int numfaces = model->numfaces;
                float *florigin = model->origin;
                float *flmins = model->mins;
                float *flmaxs = model->maxs;
                LPoint3 origin( florigin[0], florigin[1], florigin[2] );
                LPoint3 mins( flmins[0], flmins[1], flmins[2] );
                LPoint3 maxs( flmaxs[0], flmaxs[1], flmaxs[2] );

                stringstream name;
                name << "model-" << modelnum;
                PT( BSPModel ) bspmdl = new BSPModel( name.str() );
                NodePath modelroot = _result.attach_new_node( bspmdl );
                //bspmdl->set_preserve_transform( ModelNode::PT_local );
                modelroot.set_pos( ( ( mins + maxs ) / 2.0 ) + origin );

                for ( int facenum = firstface; facenum < firstface + numfaces; facenum++ )
                {
                        PT( EggData ) data = new EggData;
                        PT( EggVertexPool ) vpool = new EggVertexPool( "facevpool" );
                        data->add_child( vpool );

                        dface_t *face = &g_dfaces[facenum];

                        PT( EggPolygon ) poly = new EggPolygon;
                        data->add_child( poly );

                        texinfo_t *texinfo = &g_texinfo[face->texinfo];

                        texref_t *texref = &g_dtexrefs[texinfo->texref];
                        contents_t contents = GetTextureContents( texref->name );
                        if ( contents == CONTENTS_SKY )
                        {
                                // We don't render sky brushes.
                                continue;
                        }

                        bool skip = false;

                        PT( Texture ) tex = try_load_texref( texref );
                        string texture_name = texref->name;
                        string material = "default";
                        if ( _materials.find( texture_name ) != _materials.end() )
                        {
                                material = _materials[texture_name];
                        }
                        transform( texture_name.begin(), texture_name.end(), texture_name.begin(), tolower );

                        bool has_lighting = ( face->lightofs != -1 && _want_lightmaps ) && !skip;
                        bool has_texture = !skip;


                        LightmapPaletteDirectory::LightmapFacePaletteEntry *lmfaceentry = _lightmap_dir.face_index[facenum];

                        /* ************* FROM P3RAD ************* */

                        FaceLightmapData ld;

                        ld.mins[0] = ld.mins[1] = 999999.0;
                        ld.maxs[0] = ld.maxs[1] = -99999.0;

                        for ( int i = 0; i < face->numedges; i++ )
                        {
                                int edge_idx = g_dsurfedges[face->firstedge + i];
                                dedge_t *edge;
                                dvertex_t *vert;
                                if ( edge_idx >= 0 )
                                {
                                        edge = &g_dedges[edge_idx];
                                        vert = &g_dvertexes[edge->v[0]];
                                }
                                else
                                {
                                        edge = &g_dedges[-edge_idx];
                                        vert = &g_dvertexes[edge->v[1]];
                                }

                                LTexCoord uv = get_vertex_uv( texinfo, vert, true );

                                if ( uv.get_x() < ld.mins[0] )
                                        ld.mins[0] = uv.get_x();
                                else if ( uv.get_x() > ld.maxs[0] )
                                        ld.maxs[0] = uv.get_x();

                                if ( uv.get_y() < ld.mins[1] )
                                        ld.mins[1] = uv.get_y();
                                else if ( uv.get_y() > ld.maxs[1] )
                                        ld.maxs[1] = uv.get_y();
                        }

                        ld.texmins[0] = floor( ld.mins[0] );
                        ld.texmins[1] = floor( ld.mins[1] );
                        ld.texmaxs[0] = ceil( ld.maxs[0] );
                        ld.texmaxs[1] = ceil( ld.maxs[1] );

                        ld.texsize[0] = floor( (double)( ld.texmaxs[0] - ld.texmins[0] ) + 1 );
                        ld.texsize[1] = floor( (double)( ld.texmaxs[1] - ld.texmins[1] ) + 1 );

                        ld.midpolys[0] = ( ld.mins[0] + ld.maxs[0] ) / 2.0;
                        ld.midpolys[1] = ( ld.mins[1] + ld.maxs[1] ) / 2.0;
                        ld.midtexs[0] = ld.texsize[0] / 2.0;
                        ld.midtexs[1] = ld.texsize[1] / 2.0;

                        ld.faceentry = lmfaceentry;

                        int last_edge = face->firstedge + face->numedges;
                        int first_edge = face->firstedge;
                        for ( int j = last_edge - 1; j >= first_edge; j-- )
                        {
                                int surf_edge = g_dsurfedges[j];

                                dedge_t *edge;
                                if ( surf_edge >= 0 )
                                        edge = &g_dedges[surf_edge];
                                else
                                        edge = &g_dedges[-surf_edge];

                                if ( surf_edge < 0 )
                                {
                                        for ( int k = 0; k < 2; k++ )
                                        {
                                                PT( EggVertex ) v = make_vertex( vpool, poly, edge, texinfo,
                                                                                 face, k, &ld, tex );
                                                vpool->add_vertex( v );
                                                poly->add_vertex( v );
                                        }
                                }
                                else
                                {
                                        for ( int k = 1; k >= 0; k-- )
                                        {
                                                PT( EggVertex ) v = make_vertex( vpool, poly, edge, texinfo,
                                                                                 face, k, &ld, tex );
                                                vpool->add_vertex( v );
                                                poly->add_vertex( v );
                                        }
                                }
                        }

                        data->remove_unused_vertices( true );
                        data->remove_invalid_primitives( true );

                        poly->recompute_polygon_normal();
                        LNormald poly_normal = poly->get_normal();

                        if ( poly_normal.almost_equal( LNormald::up(), 0.5 ) )
                        {
                                // A polygon facing upwards could be considered a ground.
                                // Give it the ground bin.
                                poly->set_bin( "ground" );
                                poly->set_draw_order( 18 );
                        }

                        NodePath faceroot = _result.attach_new_node( load_egg_data( data ) );

                        if ( has_texture )
                        {
                                faceroot.set_texture( _diffuse_stage, tex );
                        }

                        if ( has_lighting )
                        {
                                faceroot.set_texture( _lightmap_stage, lmfaceentry->palette->palette_tex );
                        }

                        faceroot.wrt_reparent_to( modelroot );
                        if ( Texture::has_alpha( tex->get_format() ) )
                        {
                                bspfile_cat.info()
                                        << "Texture " << tex->get_name() << " has transparency\n";
                                faceroot.set_transparency( TransparencyAttrib::M_multisample, 1 );
                        }

                        NodePathCollection gn_npc = faceroot.find_all_matches( "**/+GeomNode" );
                        for ( int i = 0; i < gn_npc.get_num_paths(); i++ )
                        {
                                NodePath gnnp = gn_npc.get_path( i );
                                PT( GeomNode ) gn = DCAST( GeomNode, gnnp.node() );
                                for ( int j = 0; j < gn->get_num_geoms(); j++ )
                                {
                                        PT( Geom ) geom = gn->modify_geom( j );
                                        geom->set_bounds_type( BoundingVolume::BT_box );
                                        CPT( RenderAttrib ) bca = BSPFaceAttrib::make( material );
                                        CPT( RenderState ) old_state = gn->get_geom_state( j );
                                        CPT( RenderState ) new_state = old_state->add_attrib( bca, 1 );

                                        gn->set_geom( j, geom );
                                        gn->set_geom_state( j, new_state );
                                }
                        }

                        if ( skip )
                        {
                                faceroot.hide();
                        }
                }

                _model_roots.push_back( modelroot );
        }

        bspfile_cat.info()
                << "Finished making faces.\n";
}

LColor color_from_rgb_scalar( vec_t *color )
{
        double scalar = color[3];
        return LColor( color[0] * scalar / 255.0,
                       color[1] * scalar / 255.0,
                       color[2] * scalar / 255.0,
                       1.0 );
}

LColor color_from_value( const string &value, bool scale )
{
        double r, g, b, s;
        sscanf( value.c_str(), "%lf %lf %lf %lf", &r, &g, &b, &s );

        if ( scale )
        {
                r *= s / 255.0;
                g *= s / 255.0;
                b *= s / 255.0;
        }

        r /= 255.0;
        g /= 255.0;
        b /= 255.0;

        LColor col( r, g, b, 1.0 );

        return col;
}

static int extract_modelnum( entity_t *ent )
{
        string model = ValueForKey( ent, "model" );
        if ( model[0] == '*' )
        {
                return atoi( model.substr( 1 ).c_str() );
        }
        return -1;
}

void BuildGeomNodes_r( PandaNode *node, pvector<PT( GeomNode )> &list )
{
        if ( node->is_of_type( GeomNode::get_class_type() ) )
        {
                list.push_back( DCAST( GeomNode, node ) );
        }

        for ( int i = 0; i < node->get_num_children(); i++ )
        {
                BuildGeomNodes_r( node->get_child( i ), list );
        }
}

pvector<PT( GeomNode )> BuildGeomNodes( const NodePath &root )
{
        pvector<PT( GeomNode )> list;

        BuildGeomNodes_r( root.node(), list );

        return list;
}

struct VDataDef
{
        CPT( GeomVertexData ) vdata;
        CPT( Geom ) geom;
};

struct GNWG_result
{
        PT( GeomNode ) geomnode;
        int geomidx;
};

INLINE GNWG_result get_geomnode_with_geom( const NodePath &root, CPT( Geom ) geom )
{
        GNWG_result result;
        result.geomnode = nullptr;

        NodePathCollection npc = root.find_all_matches( "**/+GeomNode" );
        for ( int i = 0; i < npc.get_num_paths(); i++ )
        {
                PT( GeomNode ) gn = DCAST( GeomNode, npc.get_path( i ).node() );
                for ( int j = 0; j < gn->get_num_geoms(); j++ )
                {
                        CPT( Geom ) g = gn->get_geom( j );
                        if ( geom == g )
                        {
                                result.geomnode = gn;
                                result.geomidx = j;
                        }
                }
        }

        return result;
}

static void clear_model_nodes_below( const NodePath &top )
{
        NodePathCollection npc = top.find_all_matches( "**/+ModelNode" );
        for ( int i = 0; i < npc.get_num_paths(); i++ )
        {
                npc[i].clear_effects();
                DCAST( ModelNode, npc[i].node() )->set_preserve_transform( ModelNode::PT_drop_node );
        }
}

void BSPLoader::load_static_props()
{
        for ( size_t propnum = 0; propnum < g_dstaticprops.size(); propnum++ )
        {
                dstaticprop_t *prop = &g_dstaticprops[propnum];

                PT( BSPProp ) propnode = new BSPProp( prop->name );
                propnode->set_preserve_transform( ModelNode::PT_local );
                NodePath propnp = _result.attach_new_node( propnode );
                PT( PandaNode ) proproot = Loader::get_global_ptr()->load_sync( prop->name );
                if ( proproot == nullptr )
                {
                        bspfile_cat.warning()
                                << "Could not load static prop " << prop->name << "\n";
                        continue;
                }

                NodePath propmdl( proproot );
                LPoint3 pos;
                VectorCopy( prop->pos, pos );
                LVector3 hpr;
                VectorCopy( prop->hpr, hpr );
                LVector3 scale;
                VectorCopy( prop->scale, scale );
                propnp.set_pos( pos / 16.0 );
                propnp.set_hpr( hpr[1] - 90, hpr[0], hpr[2] );
                propnp.set_scale( scale );
                propmdl.reparent_to( propnp );
                propmdl.clear_model_nodes();
                propmdl.flatten_light();

                if ( prop->first_vertex_data != -1 )
                {
                        // prop has pre computed per vertex lighting
                        pvector<VDataDef> vdatadefs;

                        // this is actually the most annoying code in the world,
                        // since panda3d makes geoms and all its data const pointers

                        // build a list of unique GeomVertexDatas with a list of each Geom that shares it.
                        // it better be in the same order as the dstaticprop vertex datas
                        pvector<PT( GeomNode )> geomnodes = BuildGeomNodes( propmdl );
                        for ( size_t i = 0; i < geomnodes.size(); i++ )
                        {
                                PT( GeomNode ) gn = geomnodes[i];
                                for ( int j = 0; j < gn->get_num_geoms(); j++ )
                                {
                                        CPT( Geom ) geom = gn->get_geom( j );
                                        CPT( GeomVertexData ) vdata = geom->get_vertex_data();

                                        VDataDef def;
                                        def.vdata = vdata;
                                        def.geom = geom;
                                        vdatadefs.push_back( def );
                                }
                        }

                        // check for a mismatch
                        if ( vdatadefs.size() != prop->num_vertex_datas )
                        {
                                bspfile_cat.warning()
                                        << "Static prop " << prop->name << " vertex data count mismatch. "
                                        << "Will appear fullbright. "
                                        << "Has the model been changed since last map compile?\n";
                                bspfile_cat.warning()
                                        << "Loaded model has " << vdatadefs.size() << " unique vertex datas,\n"
                                        << "dstaticprop has " << prop->num_vertex_datas << "\n";
                                continue;
                        }

                        // now modulate the vertex colors to apply the lighting
                        for ( int i = 0; i < prop->num_vertex_datas; i++ )
                        {
                                dstaticpropvertexdata_t *dvdata = &g_dstaticpropvertexdatas[prop->first_vertex_data + i];
                                VDataDef *def = &vdatadefs[i];

                                PT( GeomVertexData ) mod_vdata = new GeomVertexData( *def->vdata );
                                if ( !mod_vdata->has_column( InternalName::get_color() ) )
                                {
                                        PT( GeomVertexFormat ) format = new GeomVertexFormat( *mod_vdata->get_format() );
                                        PT( GeomVertexArrayFormat ) array = format->modify_array( 0 );
                                        array->add_column( InternalName::get_color(), 4, GeomEnums::NT_uint8, GeomEnums::C_color );
                                        format->set_array( 0, array );
                                        mod_vdata->set_format( GeomVertexFormat::register_format( format ) );
                                }
                                GeomVertexRewriter color_mod( mod_vdata, InternalName::get_color() );

                                if ( dvdata->num_lighting_samples != mod_vdata->get_num_rows() )
                                {
                                        bspfile_cat.warning()
                                                << "For static prop " << prop->name << ":\n"
                                                << "number of lighting samples does not match number of vertices in vdata\n"
                                                << "number of lighting samples: " << dvdata->num_lighting_samples << "\n"
                                                << "number of vertices: " << mod_vdata->get_num_rows() << "\n";
                                        continue;
                                }

                                for ( int j = 0; j < dvdata->num_lighting_samples; j++ )
                                {
                                        colorrgbexp32_t *sample = &g_staticproplighting[dvdata->first_lighting_sample + j];
                                        LRGBColor vtx_rgb = color_shift_pixel( sample, _gamma );
                                        LColorf vtx_color( vtx_rgb[0], vtx_rgb[1], vtx_rgb[2], 1.0 );

                                        // get the current vertex color and multiply it by our lighting sample
                                        color_mod.set_row( j );
                                        LColorf curr_color( color_mod.get_data4f() );
                                        color_mod.set_row( j );
                                        curr_color.componentwise_mult( vtx_color );
                                        // now apply it to the modified vertex data
                                        color_mod.set_data4f( curr_color );
                                }

                                // apply the modified vertex data to each geom that shares this vertex data
                                CPT( Geom ) geom = def->geom;
                                GNWG_result res = get_geomnode_with_geom( propmdl, geom );
                                PT( Geom ) mod_geom = res.geomnode->modify_geom( res.geomidx );
                                mod_geom->set_vertex_data( mod_vdata );
                                res.geomnode->set_geom( res.geomidx, mod_geom );
                        }

                        // since this prop has static lighting applied to the vertices
                        // ignore any shaders, dynamic lights, or materials.
                        // also make sure the vertex colors are rendered.
                        propmdl.set_shader_off( 1 );
                        propmdl.set_light_off( 1 );
                        propmdl.set_material_off( 1 );
                        propmdl.set_attrib( ColorAttrib::make_vertex(), 2 );
                }
        }
}

void BSPLoader::load_entities()
{
        Loader *loader = Loader::get_global_ptr();

        for ( int entnum = 0; entnum < g_numentities; entnum++ )
        {
                entity_t *ent = &g_entities[entnum];

                string classname = ValueForKey( ent, "classname" );
                string id = ValueForKey( ent, "id" );

                vec_t origin[3];
                GetVectorForKey( ent, "origin", origin );

                vec_t angles[3];
                GetVectorForKey( ent, "angles", angles );

                string targetname = ValueForKey( ent, "targetname" );

                if ( classname == "env_fog" )
                {
                        // Fog
                        PN_stdfloat density = FloatForKey( ent, "fogdensity" );
                        LColor fog_color = color_from_value( ValueForKey( ent, "fogcolor" ) );
                        PT( Fog ) fog = new Fog( "env_fog" );
                        fog->set_exp_density( density );
                        fog->set_color( fog_color );
                        NodePath fognp = _render.attach_new_node( fog );
                        _render.set_fog( fog );
                        _nodepath_entities.push_back( fognp );
                }
                else if ( !strncmp( classname.c_str(), "trigger_", 8 ) ||
                          !strncmp( classname.c_str(), "func_water", 10 ) )
                {
                        // This is a bounds entity. We do not actually care about the geometry,
                        // but the mins and maxs of the model. We will use that to create
                        // a BoundingBox to check if the avatar is inside of it.
                        int modelnum = extract_modelnum( ent );
                        if ( modelnum != -1 )
                        {
                                remove_model( modelnum );

                                dmodel_t *mdl = &g_dmodels[modelnum];

                                PT( CBoundsEntity ) entity = new CBoundsEntity;
                                entity->set_data( entnum, ent, this, mdl );
                                _class_entities.push_back( ( PT( CBoundsEntity ) )entity );
#ifdef HAVE_PYTHON
                                PyObject *py_ent = DTool_CreatePyInstance<CBoundsEntity>( entity, true );
                                make_pyent( entity, py_ent, classname );
#endif
                        }
                }
                else if ( !strncmp( classname.c_str(), "func_", 5 ) )
                {
                        // Brush entites begin with func_, handle those accordingly.
                        int modelnum = extract_modelnum( ent );
                        if ( modelnum != -1 )
                        {
                                // Brush model
                                NodePath modelroot = get_model( modelnum );
                                if ( !strncmp( classname.c_str(), "func_wall", 9 ) ||
                                     !strncmp( classname.c_str(), "func_detail", 11 ) )
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
                                        continue;
                                }

                                PT( CBrushEntity ) entity = new CBrushEntity;
                                entity->set_data( entnum, ent, this, modelnum, &g_dmodels[modelnum], modelroot );
                                _class_entities.push_back( ( PT( CBaseEntity ) )entity );

#ifdef HAVE_PYTHON
                                PyObject *py_ent = DTool_CreatePyInstance<CBrushEntity>( entity, true );
                                make_pyent( entity, py_ent, classname );

#endif
                        }
                }
                else
                {
                        // We don't know what this entity is exactly, maybe they linked it to a python class.
                        // It didn't start with func_, so we can assume it's just a point entity.
                        PT( CPointEntity ) entity = new CPointEntity;
                        entity->set_data( entnum, ent, this );
                        _class_entities.push_back( ( PT( CBaseEntity ) )entity );

#ifdef HAVE_PYTHON
                        PyObject *py_ent = DTool_CreatePyInstance<CPointEntity>( entity, true );
                        make_pyent( entity, py_ent, classname );
#endif
                }
        }
}

#ifdef HAVE_PYTHON
void BSPLoader::make_pyent( CBaseEntity *cent, PyObject *py_ent, const string &classname )
{
        if ( _entity_to_class.find( classname ) != _entity_to_class.end() )
        {
                // A python class was linked to this entity!
                PyObject *obj = PyObject_CallObject( (PyObject *)_entity_to_class[classname], NULL );
                if ( obj == nullptr )
                        PyErr_PrintEx( 1 );
                PyObject_SetAttrString( obj, "cEntity", py_ent );
                PyObject_CallMethod( obj, "load", NULL );
                _py_entities.push_back( obj );
                _cent_to_pyent[cent] = obj;
        }
}
#endif

void BSPLoader::remove_model( int modelnum )
{
        NodePath modelroot = get_model( modelnum );
        if ( !modelroot.is_empty() )
        {
                _model_roots.erase( find( _model_roots.begin(), _model_roots.end(), modelroot ) );
                modelroot.remove_node();
        }
}

INLINE bool BSPLoader::is_cluster_visible( int curr_cluster, int cluster ) const
{
        if ( curr_cluster == cluster )
        {
                return true;
        }
        else if ( !_has_pvs_data )
        {
                return false;
        }

        // 1 means that the specified leaf is visible from the current leaf
        // 0 means it's not
        int dat = _leaf_pvs[curr_cluster][( cluster - 1 ) >> 3] & ( 1 << ( ( cluster - 1 ) & 7 ) );
        return dat != 0;
}

INLINE pvector<BoundingBox *> BSPLoader::get_visible_leaf_bboxs() const
{
        LightReMutexHolder holder( _leaf_aabb_lock );

        return _visible_leaf_bboxs;
}

void BSPLoader::update()
{
        // Update visibility

        LightReMutexHolder holder( _leaf_aabb_lock );

        int curr_leaf_idx = find_leaf( _camera.get_pos( _render ) );
        if ( curr_leaf_idx != _curr_leaf_idx )
        {
                _curr_leaf_idx = curr_leaf_idx;
                _visible_leaf_bboxs.clear();

                // Add ourselves to the visible list.
                _visible_leaf_bboxs.push_back( _leaf_bboxs[curr_leaf_idx] );

                if ( _vis_leafs )
                {
                        _leaf_visnp[curr_leaf_idx].set_color_scale( LColor( 0, 1, 0, 1 ), 1 );
                }

                for ( int i = 1; i < g_dmodels[0].visleafs + 1; i++ )
                {
                        if ( i == curr_leaf_idx )
                        {
                                continue;
                        }
                        const dleaf_t *leaf = &g_dleafs[i];
                        if ( is_cluster_visible( curr_leaf_idx, i ) )
                        {
                                if ( _vis_leafs )
                                {
                                        _leaf_visnp[i].set_color_scale( LColor( 0, 0, 1, 1 ), 1 );
                                }
                                _visible_leaf_bboxs.push_back( _leaf_bboxs[i] );
                        }
                        else
                        {
                                if ( _vis_leafs )
                                {
                                        _leaf_visnp[i].set_color_scale( LColor( 1, 0, 0, 1 ), 1 );
                                }
                        }
                }
        }
}

AsyncTask::DoneStatus BSPLoader::update_task( GenericAsyncTask *task, void *data )
{
        BSPLoader *self = (BSPLoader *)data;
        if ( self->_want_visibility )
        {
                self->update();
        }

        return AsyncTask::DS_cont;
}

void BSPLoader::read_materials_file()
{
        VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();

        if ( !vfs->exists( _materials_file ) )
        {
                return;
        }
        string data = vfs->read_file( _materials_file, true );

        string texname = "";
        string material = "";
        bool in_texname = true;

        for ( size_t i = 0; i < data.length(); i++ )
        {
                char c = data[i];
                if ( in_texname )
                {
                        if ( c != ' ' && c != '\t' )
                                texname += c;
                        else
                        {
                                in_texname = false;
                        }
                }
                else
                {
                        if ( c == '\n' || i == data.length() - 1 )
                        {
                                cout << "Texture `" << texname << "` is Material `" << material << "`" << endl;
                                _materials[texname] = material;
                                texname = "";
                                material = "";
                                in_texname = true;
                        }
                        else if ( c != '\r' )
                        {
                                material += c;
                        }
                }
        }
}

bool BSPLoader::read( const Filename &file )
{
        cleanup();

        if ( _gsg == nullptr )
        {
                bspfile_cat.error()
                        << "Cannot load BSP file: no GraphicsStateGuardian was specified\n";
                return false;
        }
        if ( _camera.is_empty() && _want_visibility )
        {
                bspfile_cat.error()
                        << "Cannot load BSP file: visibility requested but no Camera NodePath specified\n";
                return false;
        }
        if ( _render.is_empty() )
        {
                bspfile_cat.error()
                        << "Cannot load BSP file: no render NodePath specified\n";
                return false;
        }

        dtexdata_init();

        read_materials_file();

        PT( BSPRoot ) root = new BSPRoot( "maproot" );
        _result = NodePath( root );
        // Scale down the entire loaded level as a conversion from Hammer units to Panda units.
        // Hammer units are tiny compared to panda.
        _result.set_scale( HAMMER_TO_PANDA );
        _result.set_shader_off( 1 );
        _has_pvs_data = false;

        _leaf_pvs.resize( MAX_MAP_LEAFS );

        VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();

        bspfile_cat.info()
                << "Reading " << file.get_fullpath() << "...\n";
        nassertr( vfs->exists( file ), false );

        string data;
        nassertr( vfs->read_file( file, data, true ), false );
        int length = data.length();
        char *buffer = new char[length + 1];
        memcpy( buffer, data.c_str(), length );
        LoadBSPImage( (dheader_t *)buffer );

        ParseEntities();

        _leaf_aabb_lock.acquire();
        _leaf_bboxs.resize( MAX_MAP_LEAFS );
        // Decompress the per leaf visibility data.
        for ( int i = 0; i < g_dmodels[0].visleafs + 1; i++ )
        {
                dleaf_t *leaf = &g_dleafs[i];

                uint8_t *pvs = new uint8_t[( MAX_MAP_LEAFS + 7 ) / 8];
                memset( pvs, 0, ( g_dmodels[0].visleafs + 7 ) / 8 );

                if ( leaf->visofs != -1 )
                {
                        DecompressVis( &g_dvisdata[leaf->visofs], pvs, ( MAX_MAP_LEAFS + 7 ) / 8 );
                        _has_pvs_data = true;
                }

                _leaf_pvs[i] = pvs;

                PT( BoundingBox ) bbox = new BoundingBox(
                        LVector3( ( leaf->mins[0] - LEAF_NUDGE ) / 16.0, ( leaf->mins[1] - LEAF_NUDGE ) / 16.0, ( leaf->mins[2] - LEAF_NUDGE ) / 16.0 ),
                        LVector3( ( leaf->maxs[0] + LEAF_NUDGE ) / 16.0, ( leaf->maxs[1] + LEAF_NUDGE ) / 16.0, ( leaf->maxs[2] + LEAF_NUDGE ) / 16.0 )
                );
                _leaf_bboxs[i] = bbox;
        }
        _leaf_aabb_lock.release();

        LightmapPalettizer lmp( this );
        _lightmap_dir = lmp.palettize_lightmaps();

        make_faces();
        SceneGraphReducer gr;
        gr.apply_attribs( _result.node() );

        load_entities();
        load_static_props();

        if ( _vis_leafs )
        {
                Randomizer random;
                // Make a cube outline of the bounds for each leaf so we can visualize them.
                for ( int leafnum = 0; leafnum < g_dmodels[0].visleafs + 1; leafnum++ )
                {
                        dleaf_t *leaf = &g_dleafs[leafnum];
                        LPoint3 mins( ( leaf->mins[0] - LEAF_NUDGE ) / 16.0, ( leaf->mins[1] - LEAF_NUDGE ) / 16.0, ( leaf->mins[2] - LEAF_NUDGE ) / 16.0 );
                        LPoint3 maxs( ( leaf->maxs[0] + LEAF_NUDGE ) / 16.0, ( leaf->maxs[1] + LEAF_NUDGE ) / 16.0, ( leaf->maxs[2] + LEAF_NUDGE ) / 16.0 );
                        NodePath leafvis = _result.attach_new_node( UTIL_make_cube_outline( mins, maxs, LColor( 1, 1, 1, 1 ), 2 ) );
                        leafvis.clear_model_nodes();
                        leafvis.flatten_strong();
                        _leaf_visnp.push_back( leafvis );
                }
        }

        _amb_probe_mgr.process_ambient_probes();

        _update_task = new GenericAsyncTask( file.get_basename_wo_extension() + "-updateTask", update_task, this );
        AsyncTaskManager::get_global_ptr()->add( _update_task );

        _active_level = true;

        return true;
}

void BSPLoader::add_node_for_ambient_probes( const NodePath &node )
{
        _amb_probe_mgr.add_node( node );
}

void BSPLoader::do_optimizations()
{
        // Do some house keeping

        for ( size_t i = 0; i < _model_roots.size(); i++ )
        {
                NodePath mdlroot = _model_roots[i];
                BSPModel *mdlnode = DCAST( BSPModel, mdlroot.node() );
                if ( i == 0 )
                {
                        // We can do some hard flattening on model 0, which is just all of the
                        // static brushes that aren't tied to entities.
                        clear_model_nodes_below( mdlroot );
                        flatten_node( mdlroot );
                }
                else
                {
                	// For non zero models, the origin matters, so we will only flatten the children.
                        mdlnode->set_preserve_transform( ModelNode::PT_local );
                	NodePathCollection mdlroots = mdlroot.find_all_matches("**/+ModelNode");
                        
                	for ( int j = 0; j < mdlroots.get_num_paths(); j++ )
                	{
                                // Apply my transform to the GeomNode
                                NodePath np = mdlroots.get_path( j );
                                NodePath gnp = np.find( "**/+GeomNode" );
                                gnp.set_transform( np.get_transform() );
                                gnp.reparent_to( mdlroot );
                                np.remove_node();
                                flatten_node( gnp );
                	}
                        flatten_node( mdlroot );
                        
                }
        }


        // We can flatten the props since they just sit there.
        NodePathCollection npc = _result.find_all_matches( "**/+BSPProp" );
        for ( int i = 0; i < npc.get_num_paths(); i++ )
        {
                DCAST( BSPProp, npc[i].node() )->set_preserve_transform( ModelNode::PT_local );
                clear_model_nodes_below( npc[i] );
                npc[i].flatten_strong();
        }

        _result.premunge_scene( _gsg );
        _result.prepare_scene( _gsg );
}

void BSPLoader::set_materials_file( const Filename &file )
{
        _materials_file = file;
}

void BSPLoader::cleanup()
{
        for ( size_t i = 0; i < _model_roots.size(); i++ )
        {
                if ( !_model_roots[i].is_empty() )
                        _model_roots[i].remove_node();
        }
        _model_roots.clear();

        _materials.clear();

        _leaf_pvs.clear();

        _leaf_aabb_lock.acquire();
        _leaf_bboxs.clear();
        _visible_leaf_bboxs.clear();
        _leaf_aabb_lock.release();

        if ( _update_task != nullptr )
        {
                _update_task->remove();
                _update_task = nullptr;
        }

        _has_pvs_data = false;

        for ( size_t i = 0; i < _nodepath_entities.size(); i++ )
        {
                _nodepath_entities[i].remove_node();
        }
        _nodepath_entities.clear();

#ifdef HAVE_PYTHON
        for ( size_t i = 0; i < _py_entities.size(); i++ )
        {
                PyObject_CallMethod( _py_entities[i], "unload", NULL );
        }
        _py_entities.clear();
#endif
        _class_entities.clear();

        _lightmap_dir.face_index.clear();
        _lightmap_dir.face_entries.clear();
        _lightmap_dir.entries.clear();

        if ( !_result.is_empty() )
                _result.remove_node();

        _active_level = false;
}

BSPLoader::BSPLoader() :
        _update_task( nullptr ),
        _gsg( nullptr ),
        _has_pvs_data( false ),
        _want_visibility( true ),
        _physics_type( PT_panda ),
        _vis_leafs( false ),
        _want_lightmaps( true ),
        _curr_leaf_idx( -1 ),
        _leaf_aabb_lock( "leafAABBMutex" ),
        _gamma( DEFAULT_GAMMA ),
        _diffuse_stage( new TextureStage( "diffuse_stage" ) ),
        _lightmap_stage( new TextureStage( "lightmap_stage" ) ),
        _amb_probe_mgr( this ),
        _active_level( false )
{
        _diffuse_stage->set_texcoord_name( "diffuse" );
        _lightmap_stage->set_texcoord_name( "lightmap" );
        _lightmap_stage->set_mode( TextureStage::M_modulate );
}

INLINE bool BSPLoader::has_active_level() const
{
        return _active_level;
}

/**
 * Returns true if there is an active BSP level loaded
 * with a PVS and visibility has been toggled on.
 */
INLINE bool BSPLoader::has_visibility() const
{
        return _active_level && _want_visibility && _has_pvs_data;
}

void BSPLoader::set_camera( const NodePath &camera )
{
        _camera = camera;
}

void BSPLoader::set_render( const NodePath &render )
{
        _render = render;
}

void BSPLoader::set_want_visibility( bool flag )
{
        _want_visibility = flag;
}

void BSPLoader::set_gamma( PN_stdfloat gamma, int overbright )
{
        _gamma = gamma;
}

INLINE PN_stdfloat BSPLoader::get_gamma() const
{
        return _gamma;
}

void BSPLoader::set_gsg( GraphicsStateGuardian *gsg )
{
        _gsg = gsg;
}

void BSPLoader::set_visualize_leafs( bool flag )
{
        _vis_leafs = flag;
}

void BSPLoader::set_want_lightmaps( bool flag )
{
        _want_lightmaps = flag;
}

void BSPLoader::link_entity_to_class( const string &entname, PyTypeObject *type )
{
        _entity_to_class[entname] = type;
}

NodePath BSPLoader::get_result() const
{
        return _result;
}

int BSPLoader::get_num_entities() const
{
        return g_numentities;
}

string BSPLoader::get_entity_value( int entnum, const char *key ) const
{
        entity_t *ent = &g_entities[entnum];
        return ValueForKey( ent, key );
}

int BSPLoader::get_entity_value_int( int entnum, const char *key ) const
{
        entity_t *ent = &g_entities[entnum];
        return IntForKey( ent, key );
}

float BSPLoader::get_entity_value_float( int entnum, const char *key ) const
{
        entity_t *ent = &g_entities[entnum];
        return FloatForKey( ent, key );
}

LVector3 BSPLoader::get_entity_value_vector( int entnum, const char *key ) const
{
        entity_t *ent = &g_entities[entnum];

        vec3_t vec;
        GetVectorForKey( ent, key, vec );

        return LVector3( vec[0], vec[1], vec[2] );
}

LColor BSPLoader::get_entity_value_color( int entnum, const char *key, bool scale ) const
{
        entity_t *ent = &g_entities[entnum];

        return color_from_value( ValueForKey( ent, key ), scale );
}

NodePath BSPLoader::get_entity( int entnum ) const
{
        return _nodepath_entities[entnum];
}

NodePath BSPLoader::get_model( int modelnum ) const
{
        stringstream search;
        search << "**/model-" << modelnum;
        return _result.find( search.str() );
}

#ifdef HAVE_PYTHON
PyObject *BSPLoader::get_py_entity_by_target_name( const string &targetname ) const
{
        for ( CEntToPyEnt::const_iterator itr = _cent_to_pyent.begin(); itr != _cent_to_pyent.end(); ++itr )
        {
                CBaseEntity *cent = itr->first;
                PyObject *pyent = itr->second;
                if ( ValueForKey( &g_entities[cent->get_entnum()], "targetname" ) == targetname )
                {
                        return pyent;
                }
        }

        return nullptr;
}
#endif

void BSPLoader::set_physics_type( int type )
{
        _physics_type = type;
}

BSPLoader *BSPLoader::get_global_ptr()
{
        if ( _global_ptr == nullptr )
                _global_ptr = new BSPLoader;
        return _global_ptr;
}

/**
 * Checks if the specified bounding volume intersects any
 * of the potentially visible leaf bounding boxes.
 */
INLINE bool BSPLoader::pvs_bounds_test( const GeometricBoundingVolume *bounds )
{
        LightReMutexHolder holder( _leaf_aabb_lock );

        size_t num_aabbs = _visible_leaf_bboxs.size();
        for ( size_t i = 0; i < num_aabbs; i++ )
        {
                if ( _visible_leaf_bboxs[i]->contains( bounds ) != BoundingVolume::IF_no_intersection )
                {
                        // Bounds intersected one of the potentially visible leafs.
                        return true;
                }
        }

        // No intersections.
        return false;
}

INLINE CPT( GeometricBoundingVolume ) BSPLoader::make_net_bounds( const TransformState *net_transform,
                                                                  const GeometricBoundingVolume *original )
{
        if ( net_transform->is_identity() )
        {
                return original;
        }

        PT( GeometricBoundingVolume ) gbv = DCAST(
                GeometricBoundingVolume, original->make_copy() );
        gbv->xform( net_transform->get_mat() );
        return gbv;
}