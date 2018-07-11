#include "bsploader.h"
#include "bspfile.h"
#include "entity.h"

#include <array>
#include <bitset>
#include <math.h>

#include <asyncTaskManager.h>
#include <eggData.h>
#include <eggPolygon.h>
#include <eggVertexUV.h>
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

static PT( TextureStage ) diffuse_stage = new TextureStage( "diffuse" );
static PT( TextureStage ) lightmap_stage = new TextureStage( "lightmap" );

TypeHandle BSPGeomNode::_type_handle;

BSPGeomNode::BSPGeomNode( const string &name ) :
        GeomNode( name )
{
}

PandaNode *BSPGeomNode::make_copy()
{
        return new BSPGeomNode( *this );
}

PandaNode *BSPGeomNode::combine_with( PandaNode *other )
{
        if ( is_exact_type( get_class_type() ) &&
             other->is_exact_type( get_class_type() ) )
        {
                // Two BSPGeomNodes can combine by moving Geoms from one to the other.
                BSPGeomNode *gother = DCAST( BSPGeomNode, other );
                add_geoms_from( gother );
                return this;
        }

        return PandaNode::combine_with( other );
}

void BSPGeomNode::add_for_draw( CullTraverser *trav, CullTraverserData &data )
{
        BSPLoader *loader = BSPLoader::get_global_ptr();
        LightReMutexHolder holder( loader->_leaf_aabb_lock );

        trav->_geom_nodes_pcollector.add_level( 1 );

        // Get all the Geoms, with no decalling.
        Geoms geoms = get_geoms( trav->get_current_thread() );
        int num_geoms = geoms.get_num_geoms();
        CPT( TransformState ) internal_transform = data.get_internal_transform( trav );

        pvector<BoundingBox *> visible_leaf_aabbs = loader->get_visible_leaf_bboxs();
        size_t num_aabbs = visible_leaf_aabbs.size();

        for ( int i = 0; i < num_geoms; i++ )
        {
                CPT( Geom ) geom = geoms.get_geom( i );
                if ( geom->is_empty() )
                {
                        continue;
                }

                CPT( RenderState ) state = data._state->compose( geoms.get_geom_state( i ) );
                if ( state->has_cull_callback() && !state->cull_callback( trav, data ) )
                {
                        // Cull.
                        continue;
                }

                // Cull the individual Geom against the view frustum/leaf AABBs/cull planes.
                CPT( BoundingVolume ) geom_volume = geom->get_bounds();
                const GeometricBoundingVolume *geom_gbv =
                        DCAST( GeometricBoundingVolume, geom_volume );

                // Cull the Geom bounding volume against the view frustum andor the cull
                // planes.  Don't bother unless we've got more than one Geom, since
                // otherwise the bounding volume of the GeomNode is (probably) the same as
                // that of the one Geom, and we've already culled against that.
                if ( num_geoms > 1 )
                {
                        if ( data._view_frustum != nullptr )
                        {
                                int result = data._view_frustum->contains( geom_gbv );
                                if ( result == BoundingVolume::IF_no_intersection )
                                {
                                        // Cull this Geom.
                                        continue;
                                }
                        }
                        if ( !data._cull_planes->is_empty() )
                        {
                                // Also cull the Geom against the cull planes.
                                int result;
                                data._cull_planes->do_cull( result, state, geom_gbv );
                                if ( result == BoundingVolume::IF_no_intersection )
                                {
                                        // Cull.
                                        continue;
                                }
                        }
                }

                if ( loader->_want_visibility )
                {
                        // Now, cull the individual Geom against the visible leaf AABBs.
                        // This is for BSP culling.
                        bool intersecting_any = false;
                        for ( size_t j = 0; j < num_aabbs; j++ )
                        {
                                BoundingBox *aabb = visible_leaf_aabbs[j];
                                if ( aabb->contains( geom_gbv ) != BoundingVolume::IF_no_intersection )
                                {
                                        intersecting_any = true;
                                        break;
                                }
                        }

                        if ( !intersecting_any )
                        {
                                // The Geom did not intersect any visible leaf AABBs.
                                // Cull.
                                continue;
                        }
                }

                CullableObject *object =
                        new CullableObject( std::move( geom ), std::move( state ), internal_transform );
                trav->get_cull_handler()->record_object( object, trav );
                trav->_geoms_pcollector.add_level( 1 );
        }
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

#define DEFAULT_GAMMA 1.0
#define QRAD_GAMMA 0.55
#define DEFAULT_OVERBRIGHT 1
#define ATTN_FACTOR 0.03

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
        int num_epairs = sizeof( mapent->epairs ) / sizeof( epair_t );
}

NotifyCategoryDef( bspfile, "" );

static float lineartovertex[4096];

static void build_gamma_table( float gamma, int overbright )
{
        float f;
        float overbright_factor = 1.0;
        if ( overbright == 2 )
        {
                overbright_factor = 0.5;
        }
        else if ( overbright == 4 )
        {
                overbright_factor = 0.25;
        }

        for ( int i = 0; i < 4096; i++ )
        {
                f = pow( i / 1024.0, 1.0 / gamma );
                lineartovertex[i] = f * overbright_factor;
                if ( lineartovertex[i] > 1 )
                {
                        lineartovertex[i] = 1.0;
                }

        }
}

double linear_to_vert_light( double c )
{
        int idx = (int)( c * 1024.0 + 0.5 );
        if ( idx > 4095 )
        {
                idx = 4095;
        }
        else if ( idx < 0 )
        {
                idx = 0;
        }

        return lineartovertex[idx];
}

LRGBColor color_shift_pixel( unsigned char *sample )
{
        return LRGBColor( linear_to_vert_light( sample[0] / 255.0 ),
                          linear_to_vert_light( sample[1] / 255.0 ),
                          linear_to_vert_light( sample[2] / 255.0 ) );
}

BSPLoader *BSPLoader::_global_ptr = nullptr;


PNMImage BSPLoader::lightmap_image_from_face( dface_t *face, FaceLightmapData *ld )
{
        int width = ld->texsize[0];
        int height = ld->texsize[1];
        int num_luxels = width * height;

        if ( num_luxels <= 0 )
        {
                PNMImage img( 16, 16 );
                img.fill( 1.0 );
                return img;
        }

        PNMImage img( width, height );

        int luxel = 0;

        for ( int y = 0; y < height; y++ )
        {
                for ( int x = 0; x < width; x++ )
                {
                        LRGBColor luxel_col( 1, 1, 1 );

                        // To get the final pixel color, multiply all of the individual lightstyle samples together.
                        for ( int lightstyle = 0; lightstyle < 4; lightstyle++ )
                        {
                                if ( face->styles[lightstyle] == 0xFF )
                                {
                                        // Doesn't have this lightstyle.
                                        continue;
                                }

                                // From p3rad
                                int sample_idx = face->lightofs + lightstyle * num_luxels * 3 + luxel * 3;
                                unsigned char *sample = &g_dlightdata[sample_idx];

                                luxel_col.componentwise_mult( color_shift_pixel( sample ) );

                        }

                        img.set_xel( x, y, luxel_col );
                        luxel++;
                }
        }

        return img;
}

void BSPLoader::cull_node_path_against_leafs( NodePath &np, bool part_of_result )
{
        //np.set_attrib( BSPFaceAttrib::make( this, part_of_result ), 1 );
}

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
                        ( plane->normal[2] * pos.get_z() ) - plane->dist;

                if ( distance >= 0.0 )
                {
                        i = node->children[0];
                }
                else
                {
                        i = node->children[1];
                }

        }

        return -( i + 1 );
}

LTexCoord BSPLoader::get_vertex_uv( texinfo_t *texinfo, dvertex_t *vert ) const
{
        float *vpos = vert->point;
        LVector3 vert_pos( vpos[0], vpos[1], vpos[2] );

        LVector3 s_vec( texinfo->vecs[0][0], texinfo->vecs[0][1], texinfo->vecs[0][2] );
        float s_dist = texinfo->vecs[0][3];

        LVector3 t_vec( texinfo->vecs[1][0], texinfo->vecs[1][1], texinfo->vecs[1][2] );
        float t_dist = texinfo->vecs[1][3];

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
        LTexCoordd df_uv( uv.get_x() / df_width, -uv.get_y() / df_height );
        LTexCoordd lm_uv( ( ld->midtexs[0] + ( uv.get_x() - ld->midpolys[0] ) / TEXTURE_STEP ) / ld->texsize[0],
                          -( ld->midtexs[1] + ( uv.get_y() - ld->midpolys[1] ) / TEXTURE_STEP ) / ld->texsize[1] );

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

        vector_string extensions;
        extensions.push_back( ".jpg" );
        extensions.push_back( ".png" );
        extensions.push_back( ".bmp" );
        extensions.push_back( ".tif" );

        string name = tref->name;

        for ( size_t i = 0; i < extensions.size(); i++ )
        {
                string ext = extensions[i];
                PT( Texture ) tex = TexturePool::load_texture( name + ext );
                if ( tex != nullptr )
                {
                        bspfile_cat.info()
                                << "Loaded texref " << tref->name << "\n";
                        _texref_textures[tref] = tex;
                        return tex;
                }
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
                NodePath modelroot = _brushroot.attach_new_node( name.str() );
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
                        cout << material << endl;
                        transform( texture_name.begin(), texture_name.end(), texture_name.begin(), tolower );

                        if ( texture_name.find( "trigger" ) != string::npos )
                        {
                                continue;
                        }

                        bool has_lighting = ( face->lightofs != -1 && _want_lightmaps ) && !skip;
                        bool has_texture = !skip;

                        cout << has_lighting << endl;

                        PT( Texture ) lm_tex = new Texture( "lightmap_texture" );

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

                                LTexCoord uv = get_vertex_uv( texinfo, vert );

                                if ( uv.get_x() < ld.mins[0] )
                                        ld.mins[0] = uv.get_x();
                                else if ( uv.get_x() > ld.maxs[0] )
                                        ld.maxs[0] = uv.get_x();

                                if ( uv.get_y() < ld.mins[1] )
                                        ld.mins[1] = uv.get_y();
                                else if ( uv.get_y() > ld.maxs[1] )
                                        ld.maxs[1] = uv.get_y();
                        }

                        ld.texmins[0] = floor( ld.mins[0] / TEXTURE_STEP );
                        ld.texmins[1] = floor( ld.mins[1] / TEXTURE_STEP );
                        ld.texmaxs[0] = ceil( ld.maxs[0] / TEXTURE_STEP );
                        ld.texmaxs[1] = ceil( ld.maxs[1] / TEXTURE_STEP );

                        ld.texsize[0] = floor( (double)( ld.texmaxs[0] - ld.texmins[0] ) + 1 );
                        ld.texsize[1] = floor( (double)( ld.texmaxs[1] - ld.texmins[1] ) + 1 );

                        ld.midpolys[0] = ( ld.mins[0] + ld.maxs[0] ) / 2.0;
                        ld.midpolys[1] = ( ld.mins[1] + ld.maxs[1] ) / 2.0;
                        ld.midtexs[0] = ld.texsize[0] / 2.0;
                        ld.midtexs[1] = ld.texsize[1] / 2.0;

                        if ( has_lighting )
                        {
                                // Generate a lightmap texture from the samples
                                PNMImage lm_img = lightmap_image_from_face( face, &ld );
#ifdef EXTRACT_LIGHTMAPS
                                stringstream ss;
                                ss << "extractedLightmaps/lightmap_" << face << ".jpg";
                                PNMFileTypeJPG jpg;
                                lm_img.write( Filename( ss.str() ), &jpg );
#endif
                                lm_tex->load( lm_img );
                                lm_tex->set_magfilter( SamplerState::FT_linear );
                                lm_tex->set_minfilter( SamplerState::FT_linear_mipmap_linear );
                        }

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

                        NodePath faceroot = _brushroot.attach_new_node( load_egg_data( data ) );

                        if ( has_texture )
                        {
                                faceroot.set_texture( diffuse_stage, TexturePool::load_texture( tex->get_filename() ) );
                        }

                        if ( has_lighting )
                        {
                                faceroot.set_texture( lightmap_stage, lm_tex );
                        }

                        faceroot.wrt_reparent_to( modelroot );

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

LColor color_from_value( const string &value, bool scale = true )
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

        //return LColor( r, g, b, 1.0 );
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

                if ( classname == "light" )
                {
                        // It's a pointlight!!!

                        LColor color = color_from_value( ValueForKey( ent, "_light" ) );
                        PN_stdfloat fade = FloatForKey( ent, "_fade" );

                        PT( PointLight ) light = new PointLight( "pointLight-" + targetname );
                        light->set_color( color );
                        light->set_attenuation( LVecBase3( 1, 0, fade * ATTN_FACTOR ) );
                        NodePath light_np = _result.attach_new_node( light );
                        light_np.set_pos( origin[0], origin[1], origin[2] );

                        _render.set_light( light_np );

                        _nodepath_entities.push_back( light_np );
                }
                else if ( classname == "light_spot" )
                {
                        // Spotlight
                        PN_stdfloat cone = FloatForKey( ent, "_cone" );
                        PN_stdfloat cone2 = FloatForKey( ent, "_cone2" );
                        PN_stdfloat pitch = FloatForKey( ent, "pitch" );
                        PN_stdfloat fade = FloatForKey( ent, "_fade" );
                        LColor color = color_from_value( ValueForKey( ent, "_light" ) );

                        PT( Spotlight ) light = new Spotlight( "spotlight-" + targetname );
                        light->set_color( color );
                        //light->set_attenuation( LVecBase3( 1, 0, fade * ATTN_FACTOR ) );
                        light->get_lens()->set_fov( cone2 * 1.3 );
                        NodePath light_np = _result.attach_new_node( light );
                        // Changing the scale of a spotlight breaks it.
                        light_np.set_pos( origin[0], origin[1], origin[2] );
                        light_np.set_hpr( angles[1] - 90, angles[0] + pitch, angles[2] );
                        light_np.set_scale( _render, 1.0 );

                        _render.set_light( light_np );

                        _nodepath_entities.push_back( light_np );
                }
                else if ( classname == "light_environment" )
                {
                        // Directional light + ambient light
                        LColor sun_color = color_from_value( ValueForKey( ent, "_light" ), false );
                        LColor amb_color = color_from_value( ValueForKey( ent, "_diffuse_light" ) );

                        NodePath group = _result.attach_new_node( "light_environment-group" );

                        PT( DirectionalLight ) sun = new DirectionalLight( "directionalLight-" + targetname );
                        sun->set_color( sun_color );
                        NodePath sun_np = group.attach_new_node( sun );
                        sun_np.set_pos( origin[0], origin[1], origin[2] );
                        sun_np.set_hpr( angles[1] - 90, angles[0], angles[2] );

                        PT( AmbientLight ) amb = new AmbientLight( "ambientLight-" + targetname );
                        amb->set_color( amb_color );
                        NodePath amb_np = group.attach_new_node( amb );

                        _render.set_light( amb_np );
                        _render.set_light( sun_np );

                        _nodepath_entities.push_back( group );
                }
                else if ( classname == "prop_static" )
                {
                        // A static prop
                        string mdl_path = ValueForKey( ent, "modelpath" );
                        vec_t scale[3];
                        GetVectorForKey( ent, "scale", scale );
                        int phys_type = IntForKey( ent, "physics" );
                        bool two_sided = (bool)IntForKey( ent, "twosided" );

                        NodePath prop_np = _proproot.attach_new_node( loader->load_sync( Filename( mdl_path ) ) );
                        if ( prop_np.is_empty() )
                        {
                                bspfile_cat.warning()
                                        << "Could not load static prop " << mdl_path << "!\n";
                        }
                        else
                        {
                                bspfile_cat.info()
                                        << "Loaded static prop " << mdl_path << "...\n";
                                prop_np.set_pos( origin[0], origin[1], origin[2] );
                                prop_np.set_hpr( angles[1] - 90, angles[0], angles[2] );
                                prop_np.set_scale( scale[0] * PANDA_TO_HAMMER, scale[1] * PANDA_TO_HAMMER, scale[2] * PANDA_TO_HAMMER );
                                prop_np.set_two_sided( two_sided, 1 );
                                _nodepath_entities.push_back( prop_np );
                        }
                }
                else if ( classname == "env_fog" )
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
                else if ( !strncmp( classname.c_str(), "func_", 5 ) )
                {
                        // Brush entites begin with func_, handle those accordingly.
                        string model = ValueForKey( ent, "model" );
                        if ( model[0] == '*' )
                        {
                                // Brush model
                                int modelnum = atoi( model.substr( 1 ).c_str() );
                                NodePath modelroot = get_model( modelnum );
                                if ( !strncmp( classname.c_str(), "func_wall", 9 ) ||
                                     !strncmp( classname.c_str(), "func_detail", 11 ) )
                                {
                                        // func_walls and func_details aren't really entities,
                                        // they are just hints to the compiler. we can treat
                                        // them as regular brushes part of worldspawn.
                                        modelroot.wrt_reparent_to( get_model( 0 ) );
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

bool BSPLoader::is_cluster_visible( int curr_cluster, int cluster ) const
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

INLINE pvector<BoundingBox *> BSPLoader::get_visible_leaf_bboxs( bool render ) const
{
        LightReMutexHolder holder( _leaf_aabb_lock );

        if ( render )
                return _visible_leaf_render_bboxes;
        return _visible_leaf_bboxs;
}

void BSPLoader::update()
{
        // Update visibility

        LightReMutexHolder holder( _leaf_aabb_lock );

        int curr_leaf_idx = find_leaf( _camera.get_pos( _result ) );
        if ( curr_leaf_idx != _curr_leaf_idx )
        {
                _curr_leaf_idx = curr_leaf_idx;
                _visible_leaf_bboxs.clear();
                _visible_leaf_render_bboxes.clear();

                // Add ourselves to the visible list.
                _visible_leaf_bboxs.push_back( _leaf_bboxs[curr_leaf_idx] );
                _visible_leaf_render_bboxes.push_back( _leaf_render_bboxes[curr_leaf_idx] );

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
                                _visible_leaf_render_bboxes.push_back( _leaf_render_bboxes[i] );
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
        if ( !_materials_file.exists() )
        {
                return;
        }

        VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();
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

        _result = NodePath( "maproot" );
        // Scale down the entire loaded level as a conversion from Hammer units to Panda units.
        // Hammer units are tiny compared to panda.
        _result.set_scale( HAMMER_TO_PANDA );
        _proproot = _result.attach_new_node( "proproot" );
        _brushroot = _result.attach_new_node( "brushroot" );
        _brushroot.set_light_off( 1 );
        _brushroot.set_material_off( 1 );
        _brushroot.set_shader_off( 1 );
        _has_pvs_data = false;

        _leaf_pvs.resize( MAX_MAP_LEAFS );

        bspfile_cat.info()
                << "Reading " << file.get_fullpath() << "...\n";
        nassertr( file.exists(), false );

        VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();

        string data;
        nassertr( vfs->read_file( file, data, true ), false );
        int length = data.length();
        char *buffer = new char[length + 1];
        memcpy( buffer, data.c_str(), length );
        LoadBSPImage( (dheader_t *)buffer );

        ParseEntities();

        _leaf_aabb_lock.acquire();
        _leaf_bboxs.resize( MAX_MAP_LEAFS );
        _leaf_render_bboxes.resize( MAX_MAP_LEAFS );
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
                        LVector3( leaf->mins[0], leaf->mins[1], leaf->mins[2] ),
                        LVector3( leaf->maxs[0], leaf->maxs[1], leaf->maxs[2] )
                );
                // Also create a scaled down bbox of this leaf which would be the size of nodes relative to render.
                PT( BoundingBox ) render_bbox = new BoundingBox(
                        LVector3( leaf->mins[0] / 16.0, leaf->mins[1] / 16.0, leaf->mins[2] / 16.0 ),
                        LVector3( leaf->maxs[0] / 16.0, leaf->maxs[1] / 16.0, leaf->maxs[2] / 16.0 )
                );
                _leaf_render_bboxes[i] = render_bbox;
                _leaf_bboxs[i] = bbox;
        }
        _leaf_aabb_lock.release();

        make_faces();
        load_entities();

        if ( _vis_leafs )
        {
                Randomizer random;
                // Make a cube outline of the bounds for each leaf so we can visualize them.
                for ( int leafnum = 0; leafnum < g_dmodels[0].visleafs + 1; leafnum++ )
                {
                        dleaf_t *leaf = &g_dleafs[leafnum];
                        LPoint3 mins( leaf->mins[0], leaf->mins[1], leaf->mins[2] );
                        LPoint3 maxs( leaf->maxs[0], leaf->maxs[1], leaf->maxs[2] );
                        NodePath leafvis = _result.attach_new_node( UTIL_make_cube_outline( mins, maxs, LColor( 1, 1, 1, 1 ), 2 ) );
                        _leaf_visnp.push_back( leafvis );
                }
        }

        swap_geom_nodes( _brushroot );

        _update_task = new GenericAsyncTask( file.get_basename_wo_extension() + "-updateTask", update_task, this );
        AsyncTaskManager::get_global_ptr()->add( _update_task );

        //NodePathCollection brushfacenpc = _brushroot.find_all_matches( "**/+GeomNode" );
        //for ( int i = 0; i < brushfacenpc.get_num_paths(); i++ )
        //{
        //	DCAST( GeomNode, brushfacenpc.get_path( i ).node() )->set_bsp_mode( _want_visibility );
        //}

        return true;
}

void BSPLoader::do_optimizations()
{
        // Do some house keeping


        for ( size_t i = 0; i < _model_roots.size(); i++ )
        {
                NodePath mdlroot = _model_roots[i];
                if ( i == 0 )
                {
                        // We can do some hard flattening on model 0, which is just all of the
                        // static brushes that aren't tied to entities.
                        mdlroot.clear_model_nodes();
                        mdlroot.flatten_strong();
                }
                else
                {
                	// For non zero models, the origin matters, so we will only flatten the children.
                	NodePathCollection mdlroots = mdlroot.find_all_matches("**/+ModelRoot");
                        //DCAST( ModelRoot, mdlroot.node() )->set_preserve_transform( ModelRoot::PT_local );
                	for ( int j = 0; j < mdlroots.get_num_paths(); j++ )
                	{
                                // Apply my transform to the BSPGeomNode
                                NodePath np = mdlroots.get_path( j );
                                NodePath gnp = np.find( "**/+BSPGeomNode" );
                                gnp.set_transform( np.get_transform() );
                                gnp.reparent_to( mdlroot );
                                np.remove_node();
                                gnp.flatten_strong();
                	}
                        mdlroot.flatten_strong();
                }
        }


        // We can flatten the props since they just sit there.
        _proproot.clear_model_nodes();
        _proproot.flatten_medium();

        _result.premunge_scene( _gsg );
        _result.prepare_scene( _gsg );

        if ( !_want_visibility )
        {
                return;
        }

        NodePathCollection props = _proproot.get_children();
        for ( int i = 0; i < props.get_num_paths(); i++ )
        {
                cull_node_path_against_leafs( props.get_path( i ), true );
        }
}

void BSPLoader::set_materials_file( const Filename &file )
{
        _materials_file = file;
}

void BSPLoader::cleanup()
{
        LightReMutexHolder holder( _leaf_aabb_lock );

        for ( size_t i = 0; i < _model_roots.size(); i++ )
        {
                if ( !_model_roots[i].is_empty() )
                        _model_roots[i].remove_node();
        }
        _model_roots.clear();

        _materials.clear();

        if ( g_dlightdata != nullptr )
                dtexdata_free();

        _leaf_pvs.clear();

        _leaf_aabb_lock.acquire();
        _leaf_render_bboxes.clear();
        _leaf_bboxs.clear();
        _visible_leaf_bboxs.clear();
        _visible_leaf_render_bboxes.clear();
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
                PyObject_CallMethod( _py_entities[i], "cleanup", NULL );
        }
        _py_entities.clear();
#endif
        _class_entities.clear();


        if ( !_result.is_empty() )
                _result.remove_node();
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
        _leaf_aabb_lock( "leafAABBMutex" )
{
        diffuse_stage->set_texcoord_name( "diffuse" );
        lightmap_stage->set_mode( TextureStage::M_modulate );
        lightmap_stage->set_texcoord_name( "lightmap" );
        set_gamma( DEFAULT_GAMMA, DEFAULT_OVERBRIGHT );
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
        build_gamma_table( gamma, overbright );
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
        return _brushroot.find( search.str() );
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
 * Swaps all GeomNodes underneath this NodePath with BSPGeomNodes to effectively cull the Geoms
 * against the leaf bounding boxes.
 */
void BSPLoader::swap_geom_nodes( const NodePath &root )
{
        NodePathCollection npc = root.find_all_matches( "**/+GeomNode" );
        for ( int i = 0; i < npc.get_num_paths(); i++ )
        {
                NodePath np = npc.get_path( i );
                PT( GeomNode ) gn = DCAST( GeomNode, np.node() );
                
                PT( BSPGeomNode ) bsp_gn = new BSPGeomNode( gn->get_name() );
                bsp_gn->set_preserved( false );
                NodePath bspnp( bsp_gn );
                bspnp.reparent_to( np.get_parent() );
                bspnp.set_transform( np.get_transform() );
                bsp_gn->add_geoms_from( gn );
                bsp_gn->steal_children( gn );

                // Should be good enough.
                np.remove_node();
        }
}