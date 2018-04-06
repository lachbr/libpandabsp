#include "bspfile.h"

#include <virtualFileSystem.h>
#include <datagram.h>
#include <eggData.h>
#include <eggVertexUV.h>
#include <load_egg_file.h>
#include <texturePool.h>
#include <texture.h>
#include <nodePath.h>
#include <textureStage.h>
#include <pnmFileTypeJPG.h>
#include <asyncTaskManager.h>
#include <camera.h>
#include <lensNode.h>
#include <orthographicLens.h>

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

BSP_Vector BSPFile::read_vector( DatagramIterator &dgi )
{
        BSP_Vector vec;
        vec.x = dgi.get_float32();
        vec.y = dgi.get_float32();
        vec.z = dgi.get_float32();
        return vec;
}

BSP_DispCornerNeighbors BSPFile::read_disp_corner_neighbors( DatagramIterator &dgi )
{
        BSP_DispCornerNeighbors dcn;
        for ( int i = 0; i < 4; i++ )
        {
                dcn.neighbors[i] = dgi.get_uint16();
        }
        dcn.num_neighbors = dgi.get_uint8();

        return dcn;
}

BSP_DispSubNeighbor BSPFile::read_disp_sub_neighbor( DatagramIterator &dgi )
{
        BSP_DispSubNeighbor dsn;
        dsn.neighbor = dgi.get_uint16();
        dsn.neighbor_orientation = dgi.get_uint8();
        dsn.span = dgi.get_uint8();
        dsn.neighbor_span = dgi.get_uint8();

        return dsn;
}

BSP_DispNeighbor BSPFile::read_disp_neighbor( DatagramIterator &dgi )
{
        BSP_DispNeighbor dn;
        dn.subneighbors[0] = read_disp_sub_neighbor( dgi );
        dn.subneighbors[1] = read_disp_sub_neighbor( dgi );

        return dn;
}

void BSPFile::handle_entity_lump( DatagramIterator &dgi, int length )
{
        bspfile_cat.info()
                << "handling entity lump\n";
}

void BSPFile::handle_plane_lump( DatagramIterator &dgi, int length )
{
        int num_planes = length / sizeof( BSP_Plane );
        bspfile_cat.info()
                << "There are " << num_planes << " planes.\n";

        for ( int i = 0; i < num_planes; i++ )
        {
                BSP_Plane plane;
                plane.normal = read_vector( dgi );
                plane.distance = dgi.get_float32();
                plane.type = dgi.get_int32();

                _planes.push_back( plane );
        }
}

void BSPFile::handle_face_lump( DatagramIterator &dgi, int length )
{
        int num_faces = length / sizeof( BSP_Face );
        bspfile_cat.info()
                << "There are " << num_faces << " faces\n";

        for ( int i = 0; i < num_faces; i++ )
        {
                BSP_Face face;
                face.plane_idx = dgi.get_uint16();
                face.side = dgi.get_uint8();
                face.on_node = dgi.get_uint8();
                face.first_edge = dgi.get_int32();
                face.num_edges = dgi.get_int16();
                face.tex_info = dgi.get_int16();
                face.disp_info = dgi.get_int16();
                face.surface_fog_volume_id = dgi.get_int16();
                face.styles[0] = dgi.get_uint8();
                face.styles[1] = dgi.get_uint8();
                face.styles[2] = dgi.get_uint8();
                face.styles[3] = dgi.get_uint8();
                face.light_offset = dgi.get_int32();
                face.area = dgi.get_float32();
                face.lightmap_tex_mins[0] = dgi.get_int32();
                face.lightmap_tex_mins[1] = dgi.get_int32();
                face.lightmap_tex_size[0] = dgi.get_int32();
                face.lightmap_tex_size[1] = dgi.get_int32();
                face.orig_face = dgi.get_int32();
                face.num_prims = dgi.get_uint16();
                face.first_prim_id = dgi.get_uint16();
                face.smoothing_groups = dgi.get_uint32();

                _faces.push_back( face );
        }
}

void BSPFile::handle_origface_lump( DatagramIterator &dgi, int length )
{
        int num_origfaces = length / sizeof( BSP_Face );
        for ( int i = 0; i < num_origfaces; i++ )
        {
                BSP_Face face;
                face.plane_idx = dgi.get_uint16();
                face.side = dgi.get_uint8();
                face.on_node = dgi.get_uint8();
                face.first_edge = dgi.get_int32();
                face.num_edges = dgi.get_int16();
                face.tex_info = dgi.get_int16();
                face.disp_info = dgi.get_int16();
                face.surface_fog_volume_id = dgi.get_int16();
                face.styles[0] = dgi.get_uint8();
                face.styles[1] = dgi.get_uint8();
                face.styles[2] = dgi.get_uint8();
                face.styles[3] = dgi.get_uint8();
                face.light_offset = dgi.get_int32();
                face.area = dgi.get_float32();
                face.lightmap_tex_mins[0] = dgi.get_int32();
                face.lightmap_tex_mins[1] = dgi.get_int32();
                face.lightmap_tex_size[0] = dgi.get_int32();
                face.lightmap_tex_size[1] = dgi.get_int32();
                face.orig_face = dgi.get_int32();
                face.num_prims = dgi.get_uint16();
                face.first_prim_id = dgi.get_uint16();
                face.smoothing_groups = dgi.get_uint32();

                _orig_faces.push_back( face );
        }
}

void BSPFile::handle_brush_lump( DatagramIterator &dgi, int length )
{
        int num_brushes = length / sizeof( BSP_Brush );
        bspfile_cat.info()
                << "There are " << num_brushes << " brushes.\n";
        for ( int i = 0; i < num_brushes; i++ )
        {
                BSP_Brush brush;
                brush.first_side = dgi.get_int32();
                brush.num_sides = dgi.get_int32();
                brush.contents = dgi.get_int32();

                _brushes.push_back( brush );
        }
}

void BSPFile::handle_brushside_lump( DatagramIterator &dgi, int length )
{
        int num_brushsides = length / sizeof( BSP_BrushSide );
        for ( int i = 0; i < num_brushsides; i++ )
        {
                BSP_BrushSide bside;
                bside.plane_idx = dgi.get_uint16();
                bside.texinfo = dgi.get_int16();
                bside.dispinfo = dgi.get_int16();
                bside.bevel = dgi.get_int16();

                _brush_sides.push_back( bside );
        }
}

void BSPFile::handle_vertex_lump( DatagramIterator &dgi, int length )
{
        int num_verts = length / sizeof( BSP_Vertex );
        bspfile_cat.info()
                << "There are " << num_verts << " vertices.\n";
        for ( int i = 0; i < num_verts; i++ )
        {
                BSP_Vertex vertex;
                vertex.position = read_vector( dgi );

                bspfile_cat.info()
                        << "Vertex at position: " << vertex.position.x << ", "
                        << vertex.position.y << ", " << vertex.position.z << "\n";

                _vertices.push_back( vertex );
        }
}

void BSPFile::handle_edge_lump( DatagramIterator &dgi, int length )
{
        int num_edges = length / sizeof( BSP_Edge );
        for ( int i = 0; i < num_edges; i++ )
        {
                BSP_Edge edge;
                edge.vert_indices[0] = dgi.get_uint16();
                edge.vert_indices[1] = dgi.get_uint16();

                bspfile_cat.info()
                        << "Edge referencing verts " << edge.vert_indices[0]
                        << " and " << edge.vert_indices[1] << "\n";

                _edges.push_back( edge );
        }
}

void BSPFile::handle_dispinfo_lump( DatagramIterator &dgi, int length )
{
        int num_dispinfos = length / sizeof( BSP_DispInfo );
        for ( int i = 0; i < num_dispinfos; i++ )
        {
                BSP_DispInfo info;
                info.start_position = read_vector( dgi );
                info.disp_vert_start = dgi.get_int32();
                info.disp_tri_start = dgi.get_int32();
                info.power = dgi.get_int32();
                info.min_tesselation = dgi.get_int32();
                info.smoothing_angle = dgi.get_float32();
                info.contents = dgi.get_int32();
                info.map_face = dgi.get_uint16();
                info.lightmap_alpha_start = dgi.get_int32();
                info.lightmap_sample_position_start = dgi.get_int32();
                info.edge_neighbors[0] = read_disp_neighbor( dgi );
                info.edge_neighbors[1] = read_disp_neighbor( dgi );
                info.edge_neighbors[2] = read_disp_neighbor( dgi );
                info.edge_neighbors[3] = read_disp_neighbor( dgi );
                info.corner_neighbors[0] = read_disp_corner_neighbors( dgi );
                info.corner_neighbors[1] = read_disp_corner_neighbors( dgi );
                info.corner_neighbors[2] = read_disp_corner_neighbors( dgi );
                info.corner_neighbors[3] = read_disp_corner_neighbors( dgi );
                for ( int j = 0; j < 10; j++ )
                {
                        info.allowed_verts[j] = dgi.get_uint32();
                }

                _dispinfos.push_back( info );
        }
}

void BSPFile::handle_dispvert_lump( DatagramIterator &dgi, int length )
{
        int num_dispverts = length / sizeof( BSP_DispVert );
        for ( int i = 0; i < num_dispverts; i++ )
        {
                BSP_DispVert vert;
                vert.direction = read_vector( dgi );
                vert.dist = dgi.get_float32();
                vert.alpha = dgi.get_float32();

                _dispverts.push_back( vert );
        }
}

void BSPFile::handle_disptri_lump( DatagramIterator &dgi, int length )
{
        int num_disptris = length / sizeof( BSP_DispTri );
        for ( int i = 0; i < num_disptris; i++ )
        {
                BSP_DispTri tri;
                tri.flags = dgi.get_uint16();

                _disptris.push_back( tri );
        }
}

void BSPFile::handle_model_lump( DatagramIterator &dgi, int length )
{
        int num_models = length / sizeof( BSP_Model );
        for ( int i = 0; i < num_models; i++ )
        {
                BSP_Model mdl;
                mdl.min = read_vector( dgi );
                mdl.max = read_vector( dgi );
                mdl.origin = read_vector( dgi );
                mdl.head_node = dgi.get_int32();
                mdl.first_face = dgi.get_int32();
                mdl.num_faces = dgi.get_int32();

                _bmodels.push_back( mdl );
        }
}

void BSPFile::handle_vis_lump( DatagramIterator &dgi, int length )
{
        int num_vis = length / sizeof( BSP_Vis );
        for ( int i = 0; i < num_vis; i++ )
        {
                BSP_Vis vis;
                vis.num_clusters = dgi.get_int32();
                for ( int j = 0; j < vis.num_clusters; j++ )
                {
                        for ( int k = 0; k < 2; k++ )
                        {
                                vis.bit_offsets[j][k] = dgi.get_int32();
                        }
                }

                BSP_Lump lump = _header.lumps[4];

                DatagramIterator pvs_dgi( _dg );

                uint8_t *pvs_buffer = new uint8_t[pvs_dgi.get_remaining_size()];
                pvs_dgi.extract_bytes( pvs_buffer, pvs_dgi.get_remaining_size() );
                
                for ( int c = 0; c < vis.num_clusters; c++ )
                {
                        cout << "Decompressing pvs for cluster " << c + 1 << " / " << vis.num_clusters << endl;
                        decompress_pvs( lump.start_byte, vis.bit_offsets[c][0], pvs_buffer, vis );
                }

                delete[] pvs_buffer;
                

                cout << "Finished decompressing" << endl;

                _visibilities.push_back( vis );
        }
}

void BSPFile::handle_surfedge_lump( DatagramIterator &dgi, int length )
{
        int num_surfedges = length / sizeof( int32_t );
        for ( int i = 0; i < num_surfedges; i++ )
        {
                _surfedges[i] = dgi.get_int32();
                bspfile_cat.info() << "Surfedge " << i << " : " << _surfedges[i] << "\n";
        }
}

void BSPFile::handle_texinfo_lump( DatagramIterator &dgi, int length )
{
        int num_texinfos = length / sizeof( BSP_TexInfo );
        for ( int i = 0; i < num_texinfos; i++ )
        {
                BSP_TexInfo info;
                // Fill in the texture vecs array
                for ( int j = 0; j < 2; j++ )
                {
                        for ( int k = 0; k < 4; k++ )
                        {
                                info.texture_vecs[j][k] = dgi.get_float32();
                        }
                }
                // Do the same thing for lightmap vecs
                for ( int j = 0; j < 2; j++ )
                {
                        for ( int k = 0; k < 4; k++ )
                        {
                                info.lightmap_vecs[j][k] = dgi.get_float32();
                        }
                }
                info.flags = dgi.get_int32();
                info.texdata = dgi.get_int32();

                _texinfos.push_back( info );
        }
}

void BSPFile::handle_texdata_lump( DatagramIterator &dgi, int length )
{
        int num_texdatas = length / sizeof( BSP_TexData );
        for ( int i = 0; i < num_texdatas; i++ )
        {
                BSP_TexData data;
                data.reflectivity = read_vector( dgi );
                data.table_id = dgi.get_int32();
                data.width = dgi.get_int32();
                data.height = dgi.get_int32();
                data.view_width = dgi.get_int32();
                data.view_height = dgi.get_int32();
                
                _texdatas.push_back( data );
        }
}

void BSPFile::handle_tdst_lump( DatagramIterator &dgi, int length )
{
        int num_tds = length / sizeof( int32_t );
        for ( int i = 0; i < num_tds; i++ )
        {
                _texdata_string_table[i] = dgi.get_int32();
        }
}


void BSPFile::handle_tdsd_lump( DatagramIterator &dgi, int length )
{
        _texdata_string_data = dgi.extract_bytes( length );
}

string BSPFile::extract_texture_name( int offset )
{
        string begin = _texdata_string_data.substr( offset );
        size_t end = begin.find_first_of( '\0' );
        string texture_name = begin.substr( 0, end );
        return texture_name;
}

double tex_light_to_linear( int c, int exponent )
{
        nassertr( exponent >= -128 && exponent <= 127, 0.0 );
        return (double)c * ( pow( 2, exponent ) / 255.0 );
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

LRGBColor color_shift_pixel( BSP_ColorRGBExp32 &sample )
{
        return LRGBColor( linear_to_vert_light( tex_light_to_linear( sample.r, sample.exponent ) ),
                          linear_to_vert_light( tex_light_to_linear( sample.g, sample.exponent ) ),
                          linear_to_vert_light( tex_light_to_linear( sample.b, sample.exponent ) ) );
}

LRGBColor color_shift_lightmap( BSP_ColorRGBExp32 &sample, LRGBColor &avg_color )
{
        LRGBColor vert_color = color_shift_pixel( sample );
        LRGBColor add_color = vert_color + avg_color;

        return add_color;
}

PNMImage BSPFile::lightmap_image_from_face( BSP_Face &face )
{
        int width = face.lightmap_tex_size[0] + 1;
        int height = face.lightmap_tex_size[1] + 1;
        int num_luxels = width * height;

        int curr_x = 0;
        int curr_y = 0;

        PNMImage img( width, height );

        for ( int i = 0; i < num_luxels; i++ )
        {
                int sample_idx = ( face.light_offset / sizeof( BSP_ColorRGBExp32 ) ) + i;
                BSP_ColorRGBExp32 sample = _lightmap_samples[sample_idx];

                if ( curr_x >= width )
                {
                        curr_x = 0;
                        curr_y++;
                }

                LRGBColor luxel_col = color_shift_pixel( sample );
                img.set_xel( curr_x, curr_y, luxel_col );

                curr_x++;
        }

        return img;
}

BSP_ColorRGBExp32 BSPFile::read_lightmap_sample( DatagramIterator &dgi )
{
        BSP_ColorRGBExp32 sample;
        sample.r = dgi.get_uint8();
        sample.g = dgi.get_uint8();
        sample.b = dgi.get_uint8();
        sample.exponent = dgi.get_int8();

        return sample;
}

pvector<BSP_ColorRGBExp32> BSPFile::read_lightmap_samples( int offset, int num_samples )
{
        pvector<BSP_ColorRGBExp32> result;

        for ( int i = 0; i < num_samples; i++ )
        {
                int sample_idx = ( offset / sizeof( BSP_ColorRGBExp32 ) ) + i;
                result.push_back( _lightmap_samples[sample_idx] );
        }

        return result;
}

void BSPFile::handle_lighting_lump( DatagramIterator &dgi, int length )
{
        int num_samples = length / sizeof( BSP_ColorRGBExp32 );
        for ( int i = 0; i < num_samples; i++ )
        {
                _lightmap_samples[i] = read_lightmap_sample( dgi );
        }
}

void BSPFile::handle_leaf_lump( DatagramIterator &dgi, int length )
{
        int num_leafs = length / sizeof( BSP_Leaf );
        for ( int i = 0; i < num_leafs; i++ )
        {
                BSP_Leaf leaf;
                leaf.contents = dgi.get_int32();
                leaf.cluster = dgi.get_int16();
                leaf.area = dgi.get_int16();
                leaf.flags = dgi.get_int16();
                leaf.mins[0] = dgi.get_int16();
                leaf.mins[1] = dgi.get_int16();
                leaf.mins[2] = dgi.get_int16();
                leaf.maxs[0] = dgi.get_int16();
                leaf.maxs[1] = dgi.get_int16();
                leaf.maxs[2] = dgi.get_int16();
                leaf.first_leaf_face = dgi.get_uint16();
                leaf.num_leaf_faces = dgi.get_uint16();
                leaf.first_leaf_brush = dgi.get_uint16();
                leaf.num_leaf_brushes = dgi.get_uint16();
                leaf.leaf_water_data_id = dgi.get_int16();

                PT( BoundingBox ) bbox = new BoundingBox( LPoint3( leaf.mins[0], leaf.mins[1], leaf.mins[2] ),
                                                          LPoint3( leaf.maxs[0], leaf.maxs[1], leaf.maxs[2] ) );
                _leaf_bboxes.push_back( bbox );

                _leafs.push_back( leaf );
        }
}

void BSPFile::handle_node_lump( DatagramIterator &dgi, int length )
{
        int num_nodes = length / sizeof( BSP_Node );
        for ( int i = 0; i < num_nodes; i++ )
        {
                BSP_Node node;
                node.plane_idx = dgi.get_int32();
                node.children[0] = dgi.get_int32();
                node.children[1] = dgi.get_int32();
                node.mins[0] = dgi.get_int16();
                node.mins[1] = dgi.get_int16();
                node.mins[2] = dgi.get_int16();
                node.maxs[0] = dgi.get_int16();
                node.maxs[1] = dgi.get_int16();
                node.maxs[2] = dgi.get_int16();
                node.first_face = dgi.get_uint16();
                node.num_faces = dgi.get_uint16();
                node.area = dgi.get_int16();
                node.padding = dgi.get_int16();

                _nodes.push_back( node );
        }
}

void BSPFile::handle_leafface_lump( DatagramIterator &dgi, int length )
{
        int num_leaffaces = length / sizeof( uint16_t );
        for ( int i = 0; i < num_leaffaces; i++ )
        {
                _leaf_faces[i] = dgi.get_uint16();
        }
}

void BSPFile::handle_leafbrush_lump( DatagramIterator &dgi, int length )
{
        int num_lbs = length / sizeof( uint16_t );
        for ( int i = 0; i < num_lbs; i++ )
        {
                _leaf_brushes[i] = dgi.get_uint16();
        }
}

int BSPFile::find_leaf( const LPoint3 &pos )
{
        int i = 0;

        // Walk the BSP tree to find the index of the leaf which contains the specified
        // position.
        while ( i >= 0 )
        {
                BSP_Node &node = _nodes[i];
                BSP_Plane &plane = _planes[node.plane_idx];
                float distance = ( plane.normal.x * pos.get_x() ) +
                        ( plane.normal.y * pos.get_y() ) +
                        ( plane.normal.z * pos.get_z() ) -
                        plane.distance;

                if ( distance >= 0.0 )
                {
                        i = node.children[0];
                }     
                else
                {
                        i = node.children[1];
                }
                        
        }

        return ~i;
}

void BSPFile::read_lumps()
{
        for ( int i = 0; i < header_lumps; i++ )
        {
                BSP_Lump lump;
                lump.start_byte = _dgi.get_int32();
                lump.length = _dgi.get_int32();
                lump.version = _dgi.get_int32();
                lump.four_cc[0] = _dgi.get_uint8();
                lump.four_cc[1] = _dgi.get_uint8();
                lump.four_cc[2] = _dgi.get_uint8();
                lump.four_cc[3] = _dgi.get_uint8();

                _header.lumps[i] = lump;
        }
}

bool BSPFile::read_header()
{
        _header.ident = _dgi.get_int32();
        if ( _header.ident != vbsp_magic )
        {
                bspfile_cat.error()
                        << "This is not a VBSP file!\n";
                return false;
        }

        _header.version = _dgi.get_int32();
        read_lumps();
        _header.map_revision = _dgi.get_int32();

        return true;
}

void BSPFile::process_lumps()
{
        for ( size_t i = 0; i < header_lumps; i++ )
        {
                BSP_Lump lump = _header.lumps[i];

                DatagramIterator dgi( _dg );
                dgi.skip_bytes( lump.start_byte );

                switch ( i )
                {
                case 0:
                        // Entity lump
                        //handle_entity_lump( dgi, lump.length );
                        break;
                case 1:
                        // Plane lump
                        handle_plane_lump( dgi, lump.length );
                        break;
                case 2:
                        // Texdata lump
                        handle_texdata_lump( dgi, lump.length );
                        break;
                case 3:
                        // Vertex lump
                        handle_vertex_lump( dgi, lump.length );
                        break;
                case 4:
                        // Visibility lump
                        handle_vis_lump( dgi, lump.length );
                        break;
                case 5:
                        // Node lump
                        handle_node_lump( dgi, lump.length );
                        break;
                case 6:
                        // TexInfo lump
                        handle_texinfo_lump( dgi, lump.length );
                        break;
                case 7:
                        // Face lump
                        handle_face_lump( dgi, lump.length );
                        break;
                case 8:
                        // Lighting lump
                        handle_lighting_lump( dgi, lump.length );
                        break;
                case 10:
                        handle_leaf_lump( dgi, lump.length );
                        break;
                case 12:
                        // Edge lump
                        handle_edge_lump( dgi, lump.length );
                        break;
                case 13:
                        // Surfedge lump
                        handle_surfedge_lump( dgi, lump.length );
                        break;
                case 14:
                        // Model lump
                        //handle_model_lump( dgi, lump.length );
                        break;
                case 16:
                        // Leaf face lump
                        handle_leafface_lump( dgi, lump.length );
                        break;
                case 17:
                        handle_leafbrush_lump( dgi, lump.length );
                        break;
                case 18:
                        // Brush lump
                        handle_brush_lump( dgi, lump.length );
                        break;
                case 19:
                        // Brush side lump
                        handle_brushside_lump( dgi, lump.length );
                        break;
                case 26:
                        // DispInfo lump
                        //handle_dispinfo_lump( dgi, lump.length );
                        break;
                case 27:
                        // Orig face lump
                        handle_origface_lump( dgi, lump.length );
                        break;
                case 33:
                        // DispVerts lump
                        //handle_dispvert_lump( dgi, lump.length );
                        break;
                case 43:
                        // TexdataStringData lump
                        handle_tdsd_lump( dgi, lump.length );
                        break;
                case 44:
                        // TexdataStringTable lump
                        handle_tdst_lump( dgi, lump.length );
                        break;
                case 48:
                        // DispTris lump
                        //handle_disptri_lump( dgi, lump.length );
                        break;
                }
        }
}

PT( EggVertex ) BSPFile::make_vertex( EggVertexPool *vpool, EggPolygon *poly, BSP_Edge &edge,
                                      BSP_TexInfo &texinfo, BSP_Face &face, int k )
{
        BSP_Vertex vert = _vertices[edge.vert_indices[k]];
        BSP_Vector vpos = vert.position;
        PT( EggVertex ) v = new EggVertex;
        v->set_pos( LPoint3d( vpos.x, vpos.y, vpos.z ) );

        // Diffuse texture UV coordinates
        double duvx = ( texinfo.texture_vecs[0][0] * vpos.x ) +
                ( texinfo.texture_vecs[0][1] * vpos.y ) +
                ( texinfo.texture_vecs[0][2] * vpos.z ) +
                texinfo.texture_vecs[0][3];
        double duvy = ( texinfo.texture_vecs[1][0] * vpos.x ) +
                ( texinfo.texture_vecs[1][1] * vpos.y ) +
                ( texinfo.texture_vecs[1][2] * vpos.z ) +
                texinfo.texture_vecs[1][3];

        // Lightmap texture UV coordinates
        double luvx = ( texinfo.lightmap_vecs[0][0] * vpos.x ) +
                ( texinfo.lightmap_vecs[0][1] * vpos.y ) +
                ( texinfo.lightmap_vecs[0][2] * vpos.z ) +
                texinfo.lightmap_vecs[0][3] - face.lightmap_tex_mins[0];
        double luvy = ( texinfo.lightmap_vecs[1][0] * vpos.x ) +
                ( texinfo.lightmap_vecs[1][1] * vpos.y ) +
                ( texinfo.lightmap_vecs[1][2] * vpos.z ) +
                texinfo.lightmap_vecs[1][3] - face.lightmap_tex_mins[1];

        BSP_TexData tdat = _texdatas[texinfo.texdata];

        v->set_uv( "diffuse", LTexCoordd( duvx / tdat.view_width, -duvy / tdat.view_height ) );
        v->set_uv( "lightmap", LTexCoordd( luvx / ( face.lightmap_tex_size[0] + 1 ),
                                           -luvy / ( face.lightmap_tex_size[1] + 1 ) ) );

        return v;
}

void BSPFile::make_faces()
{
        cout << "Making faces..." << endl;
        for ( size_t i = 0; i < _faces.size(); i++ )
        {
                BSP_Face &face = _faces[i];

                PT( EggData ) data = new EggData;
                PT( EggVertexPool ) vpool = new EggVertexPool( "facevpool" );
                data->add_child( vpool );
                PT( EggPolygon ) poly = new EggPolygon;
                data->add_child( poly );

                BSP_TexInfo texinfo = _texinfos[face.tex_info];
                BSP_TexData texdata = _texdatas[texinfo.texdata];
                string texture_name = extract_texture_name( _texdata_string_table[texdata.table_id] );
                transform( texture_name.begin(), texture_name.end(), texture_name.begin(), tolower );

                if ( texture_name.find( "trigger" ) != string::npos )
                {
                        continue;
                }

                bool has_lighting = face.light_offset != -1;

                PT( Texture ) lm_tex = new Texture( "lightmap_texture" );
                if ( has_lighting )
                {
                        // Generate a lightmap texture from the samples
                        PNMImage lm_img = lightmap_image_from_face( face );
#if 0
                        stringstream ss;
                        ss << "extractedLightmaps/lightmap" << face.orig_face << ".jpg";
                        PNMFileTypeJPG jpg;
                        lm_img.write( Filename( ss.str() ), &jpg );
#endif
                        lm_tex->load( lm_img );
                        //lm_tex->set_wrap_u( SamplerState::WM_clamp );
                        //lm_tex->set_wrap_v( SamplerState::WM_clamp );
                }


                bspfile_cat.info()
                        << "Face has " << face.num_edges << " edges\n";

                int last_edge = face.first_edge + face.num_edges;
                int first_edge = face.first_edge;
                for ( int j = last_edge - 1; j >= first_edge; j-- )
                {
                        int surf_edge = _surfedges[j];

                        BSP_Edge edge;
                        if ( surf_edge > 0 )
                                edge = _edges[surf_edge];
                        else
                                edge = _edges[-surf_edge];

                        if ( surf_edge < 0 )
                        {
                                for ( int k = 0; k < 2; k++ )
                                {
                                        PT( EggVertex ) v = make_vertex( vpool, poly, edge, texinfo, face, k );
                                        vpool->add_vertex( v );
                                        poly->add_vertex( v );
                                }
                        }
                        else
                        {
                                for ( int k = 1; k >= 0; k-- )
                                {
                                        PT( EggVertex ) v = make_vertex( vpool, poly, edge, texinfo, face, k );
                                        vpool->add_vertex( v );
                                        poly->add_vertex( v );
                                }
                        }

                }

                data->recompute_polygon_normals();
                data->recompute_vertex_normals( 90.0 );
                data->remove_unused_vertices( true );
                data->remove_invalid_primitives( true );

                PT( EggTexture ) d_tex = new EggTexture( "diffuse_tex", Filename( texture_name + ".png" ) );
                d_tex->set_uv_name( "diffuse" );
                poly->add_texture( d_tex );

                PT( PandaNode ) face_node = load_egg_data( data );

                NodePath face_np( face_node );

                if ( has_lighting )
                {
                        PT( TextureStage ) lm_stage = new TextureStage( "lightmap_stage" );
                        lm_stage->set_texcoord_name( "lightmap" );
                        lm_stage->set_mode( TextureStage::M_modulate );
                        face_np.set_texture( lm_stage, lm_tex, 1 );
                }

                _face_nodes[i] = face_node;
                //_result->add_child( face_node );
                //_result->stash_child( face_node );
        }

        cout << "Finished making faces." << endl;
}

void BSPFile::make_level()
{
        cout << "Making level..." << endl;
        for ( int i = 0; i < _leafs.size(); i++ )
        {
                //cout << "Leaf " << i + 1 << " / " << _leafs.size() << endl;
                stringstream ss;
                ss << "leafRoot" << i;
                PandaNode *leaf_root = _leaf_nodes[i];
                if ( leaf_root == nullptr )
                        continue;

                BSP_Leaf &leaf = _leafs[i];

                int last_leaf_face = leaf.first_leaf_face + leaf.num_leaf_faces;
                for ( int j = leaf.first_leaf_face; j < last_leaf_face; j++ )
                {
                        uint16_t leaf_face = _leaf_faces[j];
                        PandaNode *face_node = _face_nodes[leaf_face];
                        if ( face_node == nullptr )
                                continue;
                        if ( face_node->get_num_parents() == 0 )
                        {
                                leaf_root->add_child( face_node );
                        }
                        
                }
        }

        //_result->ls(cout, 0);
}

PandaNode *BSPFile::get_result() const
{
        return _result;
}

bool BSPFile::read( const Filename &file )
{
        _result = new PandaNode( "bsproot" );

        build_gamma_table( 2.2, 1.0 );

        _surfedges = new int32_t[max_surfedges];
        _lightmap_samples = new BSP_ColorRGBExp32[max_lighting];
        _leaf_nodes = new PT( PandaNode )[max_leaffaces];
        _face_nodes = new PT( PandaNode )[max_leaffaces];

        bspfile_cat.info()
                << "Reading " << file.get_fullpath() << "...\n";
        nassertr( file.exists(), false );

        VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();

        string data;
        nassertr( vfs->read_file( file, data, true ), false );

        _dg = Datagram( data );
        _dgi = DatagramIterator( _dg );

        if ( !read_header() )
                return false;

        process_lumps();
        make_faces();
        make_bsp_tree();
        make_level();

        _update_task = new GenericAsyncTask( file.get_basename() + "-updateTask", update_task, this );
        AsyncTaskManager::get_global_ptr()->add( _update_task );

        return true;
}

void BSPFile::decompress_pvs( int lump_offset, int pvs_offset, const uint8_t *pvs_buffer, BSP_Vis &vis )
{
        uint8_t *cluster_visible = new uint8_t[vis.num_clusters];
        memset( cluster_visible, 0, vis.num_clusters );

        int v = pvs_offset;
        for ( int c = 0; c < vis.num_clusters; v++ )
        {
                if ( pvs_buffer[v] == 0 )
                {
                        v++;
                        c += 8 * pvs_buffer[v];
                }
                else
                {
                        for ( uint8_t bit = 1; bit != 0; bit *= 2, c++ )
                        {
                                if ( pvs_buffer[v] & bit )
                                {
                                        cluster_visible[c] = 1;
                                }
                        }
                }
        }

        _cluster_bitsets.push_back( cluster_visible );
}

bool BSPFile::is_cluster_visible( int curr_cluster, int cluster ) const
{
        if ( curr_cluster < 0 || cluster < 0 )
        {
                return true;
        }

        // I have absolutely no idea how this works.
        return ( ( _cluster_bitsets[curr_cluster][cluster >> 3] & ( 1 << ( cluster & 7 ) ) ) != 0 );
}

PT( PandaNode ) BSPFile::process_node( int node_idx )
{
        BSP_Node &bspnode = _nodes[node_idx];

        stringstream ss;
        ss << "node-" << node_idx;
        PT( PandaNode ) node = new PandaNode( ss.str() );

        // Process the two children of this node.

        if ( bspnode.children[0] < 0 )
        {
                // when the child index is negative, it means it's a leaf.
                // the leaf index is then a bitwise negation.
                int leaf_idx = ~bspnode.children[0];
                stringstream ss;
                ss << "leaf-" << leaf_idx;
                PT( PandaNode ) leaf_node = new PandaNode( ss.str() );
                _leaf_nodes[leaf_idx] = leaf_node;
                node->add_child( leaf_node );
        }
        else
        {
                int childnode_idx = bspnode.children[0];
                node->add_child( process_node( childnode_idx ) );
        }

        if ( bspnode.children[1] < 0 )
        {
                int leaf_idx = ~bspnode.children[1];
                stringstream ss;
                ss << "leaf-" << leaf_idx;
                PT( PandaNode ) leaf_node = new PandaNode( ss.str() );
                _leaf_nodes[leaf_idx] = leaf_node;
                node->add_child( leaf_node );
        }
        else
        {
                int childnode_idx = bspnode.children[1];
                node->add_child( process_node( childnode_idx ) );
        }

        return node;
}

void BSPFile::make_bsp_tree()
{
        cout << "Making bsp tree..." << endl;
        _bsp_head_node = process_node( 0 );
        _result->add_child( _bsp_head_node );
}

void BSPFile::update()
{
        // Update visibility

        //cout << "update" << endl;

        //nassertv( !_camera.is_empty() );

        //pvector<PandaNode *> already_visible;
        //_visible_faces.clear();

        int curr_leaf_idx = find_leaf( _camera_group.get_pos( NodePath( _result ) ) );
        int curr_cluster = _leafs[curr_leaf_idx].cluster;

        //int clusters_skipped = 0;
        //int faces_skipped = 0;
        //int leafs_culled = 0;
        //int faces_drawn = 0;

        PT( BoundingVolume ) cam_bounds = DCAST( Camera, _camera.node() )->get_lens()->make_bounds();
        //cam_bounds->output( cout );

        //for ( size_t i = 0; i < _face_nodes.size(); i++ )
        //{
        //        _result->stash_child( _face_nodes[i] );
        //}

        for ( int i = _leafs.size() - 1; i >= 0; i-- )
        {
                //cout << "Leaf " << i + 1 << " / " << _leafs.size() << endl;
                BSP_Leaf &leaf = _leafs[i];
                PandaNode *leaf_node = _leaf_nodes[i];
                if ( leaf_node == nullptr )
                        continue;
                if ( !is_cluster_visible( curr_cluster, leaf.cluster ) )
                {
                        NodePath( leaf_node ).hide();
                        //clusters_skipped++;
                        continue;
                }

                BoundingBox *bbox = _leaf_bboxes[i];
                bbox->xform( _camera_group.get_mat() );
                bbox->output( cout );
                if ( !cam_bounds->contains( bbox ) )
                {
                        NodePath( leaf_node ).hide();
                        //leafs_culled++;
                        continue;
                }

                NodePath( leaf_node ).show();
        }
}

AsyncTask::DoneStatus BSPFile::update_task( GenericAsyncTask *task, void *data )
{
        BSPFile *self = (BSPFile *)data;
        self->update();

        return AsyncTask::DS_cont;
}

void BSPFile::set_camera( const NodePath &cam, const NodePath &camera_group )
{
        _camera = cam;
        _camera_group = camera_group;
}