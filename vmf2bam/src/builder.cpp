#include "builder.h"
#include <vifParser.h>

#include <string>

#include <eggData.h>
#include <eggVertexPool.h>
#include <eggPolygon.h>
#include <eggVertex.h>
#include <eggVertexUV.h>
#include <load_egg_file.h>

#include <geomVertexData.h>
#include <geomVertexWriter.h>
#include <geomTriangles.h>
#include <geomNode.h>
#include <geomVertexFormat.h>
#include <geom.h>

#include <shadeModelAttrib.h>

#include <orthographicLens.h>

#include <occluderNode.h>

#include <texturePool.h>

#include <fog.h>

#include <ambientLight.h>
#include <directionalLight.h>

#include <collisionBox.h>
#include <collisionNode.h>
#include <collisionPolygon.h>

#include <pointLight.h>
#include <spotlight.h>

#include <bitMask.h>

#include <algorithm>

#include <door.h>
#include <mapInfo.h>
#include <sun.h>
#include <devshotCamera.h>

#define FLR_BITMASK BitMask32(2)
#define WALL_BITMASK BitMask32(1)
#define EVT_BITMASK BitMask32(3)

#define FLOOR_NORMAL_LEEWAY 0.5
#define WALL_NORMAL_LEEWAY 0.72

static WindowFramework *pWindow = NULL;

static const double pi = 3.14159265359;
static const PN_stdfloat scene_scale = 0.075;

typedef vector<LVecBase3f> Quad;
typedef vector<Quad> displacedplane_t;

struct DispVert
{
        int index;
        LVecBase3f pos;
        LVecBase3f normal;
        float offset; // Relative to the normal
        LVecBase2f uv_coord;
        LVecBase3f sm_vert_normal;
};
typedef vector<DispVert> DispVertVec;
typedef vector<DispVertVec> DispPlane;

DispVert *get_disp_vert_by_index( DispPlane &plane, int index )
{
        for ( size_t i = 0; i < plane.size(); i++ )
        {
                DispVertVec *quad = &plane[i];
                for ( size_t j = 0; j < quad->size(); j++ )
                {
                        DispVert *d_vert = &quad->at( j );
                        if ( d_vert->index == index )
                        {
                                return d_vert;
                        }
                }
        }

        return NULL;
}

LVector3 get_major_axis( LVector3 &vec )
{

        PN_stdfloat abx = abs( vec[0] );
        PN_stdfloat aby = abs( vec[1] );
        PN_stdfloat abz = abs( vec[2] );

        if ( abx >= aby && abx >= abz )
        {
                return abx > 0.0 ? LVector3::right() : LVector3::left();
        }
        else if ( aby >= abx && aby >= abz )
        {
                return aby > 0.0 ? LVector3::forward() : LVector3::back();
        }
        else if ( abz >= abx && abz >= aby )
        {
                return abz > 0.0 ? LVector3::up() : LVector3::down();
        }

        // Somehow, none of the conditions were met.
        return vec;
}

LPoint3d to_point3d( LVecBase3f &vec )
{
        return LPoint3d( vec.get_x(), vec.get_y(), vec.get_z() );
}

LTexCoordd to_texcoord( LVecBase2f &vec )
{
        return LTexCoordd( vec.get_x(), vec.get_y() );
}

PT( EggData )
make_brush_side( const string &side_id, Quad &verts )
{
        PT( EggData ) data = new EggData;
        PT( EggVertexPool ) vpool = new EggVertexPool( "side." + side_id + ".vpool" );
        data->add_child( vpool );

        PT( EggVertex ) vert_a, vert_b, vert_c, vert_d;

        vert_a = new EggVertex;
        vert_a->set_pos( to_point3d( verts[0] ) );
        vert_a->set_uv( LTexCoordd( 0, 1 ) );

        vert_b = new EggVertex;
        vert_b->set_pos( to_point3d( verts[1] ) );
        vert_b->set_uv( LTexCoordd( 1, 1 ) );

        vert_c = new EggVertex;
        vert_c->set_pos( to_point3d( verts[2] ) );
        vert_c->set_uv( LTexCoordd( 1, 0 ) );

        vert_d = new EggVertex;
        vert_d->set_pos( to_point3d( verts[3] ) );
        vert_d->set_uv( LTexCoordd( 0, 0 ) );

        vpool->add_vertex( vert_a );
        vpool->add_vertex( vert_b );
        vpool->add_vertex( vert_c );
        vpool->add_vertex( vert_d );

        PT( EggPolygon ) poly = new EggPolygon;
        poly->add_vertex( vert_a );
        poly->add_vertex( vert_b );
        poly->add_vertex( vert_c );
        poly->add_vertex( vert_d );

        data->add_child( poly );

        data->recompute_polygon_normals();
        data->recompute_vertex_normals( 90.0 );
        data->remove_unused_vertices( true );

        //poly->set_normal(-poly->get_normal());
        //vert_a->set_normal(-vert_a->get_normal());
        //vert_b->set_normal(-vert_b->get_normal());
        //vert_c->set_normal(-vert_c->get_normal());
        //vert_d->set_normal(-vert_d->get_normal());

        return data;
}

LVecBase3f hpr_from_normal( LVecBase3f &normal )
{
        return normal;
}

LVecBase3f get_displaced_vert_pos( DispVert &d_vert )
{
        return d_vert.pos + ( d_vert.normal * d_vert.offset );
}

bool is_facing_up( LVecBase3f &norm )
{
        return norm.almost_equal( LVector3::up(), FLOOR_NORMAL_LEEWAY );
}

bool is_facing_down( LVecBase3f &norm )
{
        return norm.almost_equal( LVector3::down(), FLOOR_NORMAL_LEEWAY );
}

bool is_facing_forward( LVecBase3f &norm )
{
        return norm.almost_equal( LVector3::forward(), WALL_NORMAL_LEEWAY );
}

bool is_facing_backward( LVecBase3f &norm )
{
        return norm.almost_equal( LVector3::back(), WALL_NORMAL_LEEWAY );
}

bool is_facing_left( LVecBase3f &norm )
{
        return norm.almost_equal( LVector3::left(), WALL_NORMAL_LEEWAY );
}

bool is_facing_right( LVecBase3f &norm )
{
        return norm.almost_equal( LVector3::right(), WALL_NORMAL_LEEWAY );
}

LVecBase3f calc_vert_normal()
{
        return LVector3::up();
}

LVecBase3f calc_face_normal( Quad &verts )
{
        LVecBase3f normal( 0, 0, 0 );

        for ( size_t i = 0; i < verts.size(); i++ )
        {
                int j = ( i + 1 ) % 4;
                normal.set_x( normal.get_x() + ( ( verts[i][1] - verts[j][1] ) * ( verts[i][2] + verts[j][2] ) ) );
                normal.set_y( normal.get_y() + ( ( verts[i][2] - verts[j][2] ) * ( verts[i][0] + verts[j][0] ) ) );
                normal.set_z( normal.get_z() + ( ( verts[i][0] - verts[j][0] ) * ( verts[i][1] + verts[j][1] ) ) );
        }

        normal = normal.normalized();

        // now flip the normals.
        normal.set_x( -normal.get_x() );
        normal.set_y( -normal.get_y() );
        normal.set_z( -normal.get_z() );

        return normal;
}

enum SolidType
{
        ST_wall,
        ST_floor
};

int get_solid_type( Quad &verts )
{
        LVecBase3f normal = calc_face_normal( verts );

        if ( is_facing_up( normal ) )
        {
                return ST_floor;
        }

        return ST_wall;
}

vector<LVecBase2f> calc_uvs( vector<LVecBase3f> verts, LVecBase3f &normal )
{
        vector<LVecBase2f> result;

        LVecBase2f uv1;
        LVecBase2f uv2;
        LVecBase2f uv3;
        LVecBase2f uv4;

        if ( is_facing_left( normal ) || is_facing_right( normal ) )
        {
                uv1 = LVecBase2f( verts[0].get_y(), verts[0].get_z() ).normalized();
                uv2 = LVecBase2f( verts[1].get_y(), verts[1].get_z() ).normalized();
                uv3 = LVecBase2f( verts[2].get_y(), verts[2].get_z() ).normalized();
                uv4 = LVecBase2f( verts[3].get_y(), verts[3].get_z() ).normalized();
        }
        else if ( is_facing_forward( normal ) || is_facing_backward( normal ) )
        {
                uv1 = LVecBase2f( verts[0].get_x(), verts[0].get_z() ).normalized();
                uv2 = LVecBase2f( verts[1].get_x(), verts[1].get_z() ).normalized();
                uv3 = LVecBase2f( verts[2].get_x(), verts[2].get_z() ).normalized();
                uv4 = LVecBase2f( verts[3].get_x(), verts[3].get_z() ).normalized();
        }
        else if ( is_facing_up( normal ) || is_facing_down( normal ) )
        {
                uv1 = LVecBase2f( verts[0].get_x(), verts[0].get_y() ).normalized();
                uv2 = LVecBase2f( verts[1].get_x(), verts[1].get_y() ).normalized();
                uv3 = LVecBase2f( verts[2].get_x(), verts[2].get_y() ).normalized();
                uv4 = LVecBase2f( verts[3].get_x(), verts[3].get_y() ).normalized();
        }
        else
        {
                uv1 = LVecBase2f( verts[0].get_x(), verts[0].get_y() ).normalized();
                uv2 = LVecBase2f( verts[1].get_x(), verts[1].get_y() ).normalized();
                uv3 = LVecBase2f( verts[2].get_x(), verts[2].get_y() ).normalized();
                uv4 = LVecBase2f( verts[3].get_x(), verts[3].get_y() ).normalized();
        }

        result.push_back( uv1 );
        result.push_back( uv2 );
        result.push_back( uv3 );
        result.push_back( uv4 );

        return result;
}

struct DispCollQuad
{
        PT( CollisionPolygon ) tri1;
        PT( CollisionPolygon ) tri2;
};

DispCollQuad make_coll_poly_disp( DispVertVec &verts )
{

        PT( CollisionPolygon ) poly1 = new CollisionPolygon( get_displaced_vert_pos( verts[3] ), get_displaced_vert_pos( verts[0] ),
                                                             get_displaced_vert_pos( verts[1] ) );
        poly1->set_tangible( true );

        PT( CollisionPolygon ) poly2 = new CollisionPolygon( get_displaced_vert_pos( verts[2] ), get_displaced_vert_pos( verts[3] ),
                                                             get_displaced_vert_pos( verts[1] ) );
        poly2->set_tangible( true );

        DispCollQuad result;
        result.tri1 = poly1;
        result.tri2 = poly2;
        return result;
}

// Power: number of subdivisions
DispPlane create_displacement( Quad &quad, int nPower, Parser *p, Object &objDisp )
{
        DispPlane result;

        LVecBase3f a = quad[0];
        LVecBase3f b = quad[1];
        LVecBase3f c = quad[2];
        LVecBase3f d = quad[3];

        LVecBase3f normal = calc_face_normal( quad );

        float flHeight;
        if ( is_facing_up( normal ) )
        {
                flHeight = abs( a[1] - c[1] );
        }
        else
        {
                flHeight = abs( a[2] - c[2] );
        }


        float flWidth;
        if ( is_facing_up( normal ) ||
             is_facing_forward( normal ) ||
             is_facing_backward( normal ) )
        {
                flWidth = abs( a[0] - c[0] );
        }
        else
        {
                flWidth = abs( a[1] - c[1] );
        }

        int nQuads;
        if ( nPower == 3 )
        {
                nQuads = ( nPower * nPower ) - 1;
        }
        else
        {
                nQuads = ( nPower * nPower );
        }

        float flRowHeight = flHeight / nQuads;
        float flColWidth = flWidth / nQuads;


        int nColsPerRow;
        if ( nPower == 3 )
        {
                nColsPerRow = ( nPower * nPower );
        }
        else
        {
                nColsPerRow = ( nPower * nPower ) + 1;
        }
        int nRows = nColsPerRow;

        int nVerts = nColsPerRow * nColsPerRow;

        int vertIndex = nVerts;

        DispVertVec allVerts;

        Object objNormal = p->get_object_with_name( objDisp, "normals" );
        Object objDistances = p->get_object_with_name( objDisp, "distances" );

        vector<float> normals;
        vector<float> offsets;

        for ( int i = 0; i < nColsPerRow; i++ )
        {
                stringstream ss;
                ss << "row" << i;
                vector<float> rowNorms = Parser::parse_float_list_str( p->get_property_value( objNormal, ss.str() ) );
                vector<float> rowOffs = Parser::parse_float_list_str( p->get_property_value( objDistances, ss.str() ) );
                for ( size_t j = 0; j < rowNorms.size(); j++ )
                {
                        normals.push_back( rowNorms[j] );
                }
                for ( size_t j = 0; j < rowOffs.size(); j++ )
                {
                        offsets.push_back( rowOffs[j] );
                }
        }


        int iRow;

        if ( is_facing_backward( normal ) )
        {
                iRow = nRows - 1;
        }
        else
        {
                iRow = 0;
        }

        while ( true )
        {
                if ( is_facing_backward( normal ) )
                {
                        if ( iRow < 0 )
                        {
                                break;
                        }
                }
                else
                {
                        if ( iRow > nRows - 1 )
                        {
                                break;
                        }
                }

                float flRowBot = iRow * flRowHeight;
                float flRowTop = flRowBot + flRowHeight;

                int iCol;
                if ( is_facing_backward( normal ) )
                {
                        iCol = 0;
                }
                else
                {
                        iCol = nColsPerRow - 1;
                }
                while ( true )
                {
                        if ( is_facing_backward( normal ) )
                        {
                                if ( iCol > nColsPerRow - 1 )
                                {
                                        break;
                                }
                        }
                        else
                        {
                                if ( iCol < 0 )
                                {
                                        break;
                                }
                        }

                        float flColLeft = iCol * flColWidth;
                        float flColRight = flColLeft + flColWidth;

                        DispVert vert;

                        vert.uv_coord = LVecBase2f( flColLeft / ( ( nColsPerRow - 1 ) * flColWidth ),
                                                    flRowBot / ( ( nColsPerRow - 1 ) * flRowHeight ) );

                        if ( is_facing_up( normal ) )
                        {
                                vert.index = --vertIndex;
                                vert.pos = LVecBase3f( a[0] + flColLeft, a[1] - flRowBot, a[2] );
                                iCol--;
                        }
                        else if ( is_facing_forward( normal ) )
                        {
                                vert.index = ( nColsPerRow * iCol ) + iRow;
                                //cout << vert.index << endl;
                                vert.pos = LVecBase3f( a[0] + flColLeft, a[1], a[2] + flRowBot );
                                iCol--;
                        }
                        else if ( is_facing_backward( normal ) )
                        {
                                vert.index = --vertIndex;
                                vert.pos = LVecBase3f( a[0] - flColLeft, a[1], a[2] + flRowBot );
                                iCol++;
                        }
                        else if ( is_facing_left( normal ) )
                        {
                                vert.index = ( nColsPerRow * iCol ) + iRow;
                                vert.pos = LVecBase3f( a[0], a[1] + flColLeft, a[2] + flRowBot );
                                iCol--;
                        }
                        else if ( is_facing_right( normal ) )
                        {
                                vert.index = ( nVerts - 1 ) - ( --vertIndex );
                                vert.pos = LVecBase3f( a[0], a[1] - flColLeft, a[2] + flRowBot );
                                iCol--;
                        }

                        vert.offset = offsets[vert.index];
                        vert.normal = LVecBase3f( normals[vert.index * 3], normals[( vert.index * 3 ) + 1], normals[( vert.index * 3 ) + 2] );
                        //cout << vert.normal << endl;

                        allVerts.push_back( vert );
                }

                if ( is_facing_backward( normal ) )
                {
                        iRow--;
                }
                else
                {
                        iRow++;
                }
        }

        for ( int i = 0; i < nVerts; i++ )
        {
                int nRow = i / nColsPerRow;

                // Skip this quad if we're talking about the final row of the
                // displacement plane or the last column of a row.
                //
                // If we don't skip, we would be making a quad using undefined vertices.
                // Starting at the last column of a row as vert A, and then going over to the
                // left for vert B, but there's no vertex to the left of vert A since vert A
                // is the last column.
                //
                // - - - - - <
                // - - - - - <
                // - - - - - < skip these columns
                // - - - - - <
                // - - - - - <
                // ^ ^ ^ ^ ^
                // skip this row
                if ( i == ( ( nRow * nColsPerRow ) + ( nColsPerRow - 1 ) ) || nRow >= nRows - 1 )
                {
                        continue;
                }

                DispVertVec newQuad;
                newQuad.push_back( allVerts[i] );
                newQuad.push_back( allVerts[i + 1] );
                newQuad.push_back( allVerts[( i + nColsPerRow ) + 1] );
                newQuad.push_back( allVerts[i + nColsPerRow] );

                result.push_back( newQuad );
        }

        for ( int i = 0; i < nVerts; i++ )
        {
                DispVert vert = allVerts[i];
                DispPlane quadsSharingVert;

                LVector3f sm_vert_normal( 0, 0, 0 );

                for ( size_t j = 0; j < result.size(); j++ )
                {
                        DispVertVec quad = result[j];
                        if ( find_if( quad.begin(), quad.end(), [vert]( const DispVert & other )
                {
                        return other.index == vert.index;
                } ) != quad.end() )
                        {
                                quadsSharingVert.push_back( quad );
                        }
                }

                for ( size_t j = 0; j < quadsSharingVert.size(); j++ )
                {
                        DispVertVec quad = quadsSharingVert[j];

                        LVecBase3f dA, dB, dC, dD;
                        dA = get_displaced_vert_pos( quad[0] );
                        dB = get_displaced_vert_pos( quad[1] );
                        dC = get_displaced_vert_pos( quad[2] );
                        dD = get_displaced_vert_pos( quad[3] );

                        Quad qv;
                        qv.push_back( dA );
                        qv.push_back( dB );
                        qv.push_back( dC );
                        qv.push_back( dD );

                        sm_vert_normal += calc_face_normal( qv );
                }

                //cout << "Num quads with this vert: " << quadsSharingVert.size() << endl;

                double dLen = quadsSharingVert.size();

                sm_vert_normal /= dLen;

                vert.sm_vert_normal = -sm_vert_normal.normalized();
                //cout << "Vertex smooth shading normal:" << endl;
                cout << vert.sm_vert_normal << endl;
        }

        return result;
}

PT( EggData )
handle_displacement_side( Parser *p, Object &side_obj, const string &side_id,
                          PT( CollisionNode ) &coll_node, Quad &verts )
{
        cout << "Making displacement brush." << endl;

        Object objDisp = p->get_object_with_name( side_obj, "dispinfo" );
        int nPower = stoi( p->get_property_value( objDisp, "power" ) );

        cout << "power of " << nPower << endl;

        int nRows;
        if ( nPower == 3 )
        {
                nRows = ( nPower * nPower );
        }
        else
        {
                nRows = ( nPower * nPower ) + 1;
        }

        int nVerts = nRows * nRows;

        int nQuadsPerRow = nRows - 1;

        PT( EggData ) pData = new EggData;
        PT( EggVertexPool ) pVPool = new EggVertexPool( "disp." + side_id + ".vpool" );
        pData->add_child( pVPool );

        //PT( GeomVertexData ) vert_data = new GeomVertexData( "sidedatadisp." + sideId,
        //GeomVertexFormat::get_v3n3t2(), Geom::UH_static );
        //vert_data->set_num_rows( nVerts );

        //GeomVertexWriter v_writer( vert_data, InternalName::get_vertex() );

        //GeomVertexWriter v_writer_norm( vert_data, InternalName::get_normal() );

        //GeomVertexWriter v_writer_tex( vert_data, InternalName::get_texcoord() );

        LVecBase3f normal = calc_face_normal( verts );
        cout << "Normal of displacement plane: " << normal << endl;



        coll_node = new CollisionNode( "sidecolldisp." + side_id );
        if ( is_facing_up( normal ) )
        {
                coll_node->set_collide_mask( FLR_BITMASK );
        }
        else
        {
                coll_node->set_collide_mask( WALL_BITMASK );
        }



        //PT( Geom ) geo = new Geom( vert_data );

        DispPlane dispPlane = create_displacement( verts, nPower, p, objDisp );

        int vertOrder = 1;

        for ( size_t l = 0; l < dispPlane.size(); l++ )
        {
                DispVertVec quad = dispPlane[l];

                LVecBase3f dA, dB, dC, dD;
                dA = get_displaced_vert_pos( quad[0] );
                dB = get_displaced_vert_pos( quad[1] );
                dC = get_displaced_vert_pos( quad[2] );
                dD = get_displaced_vert_pos( quad[3] );

                Quad qv;
                qv.push_back( dA );
                qv.push_back( dB );
                qv.push_back( dC );
                qv.push_back( dD );

                PT( EggVertex ) pA = new EggVertex;
                pA->set_pos( to_point3d( dA ) );
                pA->set_uv( to_texcoord( quad[0].uv_coord ) );

                PT( EggVertex ) pB = new EggVertex;
                pB->set_pos( to_point3d( dB ) );
                pB->set_uv( to_texcoord( quad[1].uv_coord ) );

                PT( EggVertex ) pC = new EggVertex;
                pC->set_pos( to_point3d( dC ) );
                pC->set_uv( to_texcoord( quad[2].uv_coord ) );

                PT( EggVertex ) pD = new EggVertex;
                pD->set_pos( to_point3d( dD ) );
                pD->set_uv( to_texcoord( quad[3].uv_coord ) );

                pVPool->add_vertex( pA );
                pVPool->add_vertex( pB );
                pVPool->add_vertex( pC );
                pVPool->add_vertex( pD );

                PT( EggPolygon ) pPoly = new EggPolygon;
                pData->add_child( pPoly );
                pPoly->add_vertex( pA );
                pPoly->add_vertex( pB );
                pPoly->add_vertex( pC );
                pPoly->add_vertex( pD );

                pData->recompute_polygon_normals();
                pData->recompute_vertex_normals( 90.0 );
                pData->remove_unused_vertices( true );

                //v_writer_norm.add_data3f( quad[0].smVertNormal );
                //v_writer_norm.add_data3f( quad[1].smVertNormal );
                //v_writer_norm.add_data3f( quad[2].smVertNormal );
                //v_writer_norm.add_data3f( quad[3].smVertNormal );

                //v_writer.add_data3f( dA );
                //v_writer.add_data3f( dB );
                //v_writer.add_data3f( dC );
                //v_writer.add_data3f( dD );

                //v_writer_tex.add_data2f( quad[0].uvCoord );
                //v_writer_tex.add_data2f( quad[1].uvCoord );
                //v_writer_tex.add_data2f( quad[2].uvCoord );
                //v_writer_tex.add_data2f( quad[3].uvCoord );

                //PT( GeomTriangles ) tris = new GeomTriangles( Geom::UH_static );
                //tris->set_shade_model( GeomEnums::SM_smooth );
                //if ( vertOrder == 1 )
                //{
                //        tris->add_vertices( ( l * 4 ) + 3, ( l * 4 ), ( l * 4 ) + 2 );
                //        tris->add_vertices( ( l * 4 ), ( l * 4 ) + 1, ( l * 4 ) + 2 );
                //}
                //else
                //{
                //        tris->add_vertices( ( l * 4 ) + 3, ( l * 4 ), ( l * 4 ) + 1 );
                //        tris->add_vertices( ( l * 4 ) + 2, ( l * 4 ) + 3, ( l * 4 ) + 1 );
                //}

                int nRow = l / nQuadsPerRow;
                int nLastColOfRow = ( ( nQuadsPerRow * nRow ) + ( ( nQuadsPerRow - nRow ) - 1 ) ) + nRow;

                if ( l != nLastColOfRow )
                {
                        vertOrder = -vertOrder;
                }

                //tris->close_primitive();

                //geo->add_primitive( tris );

                DispCollQuad coll_quad = make_coll_poly_disp( quad );
                coll_node->add_solid( coll_quad.tri1 );
                coll_node->add_solid( coll_quad.tri2 );
        }

        //side = new GeomNode( "sidedisp." + sideId );
        //side->add_geom( geo );

        return pData;
}

vector<LVecBase3f> make_vectors( vector<vector<int>> &verts_pos )
{
        vector<LVecBase3f> result;

        LVecBase3f vert1 = LVecBase3f( verts_pos[0][0], verts_pos[0][1], verts_pos[0][2] );
        LVecBase3f vert2 = LVecBase3f( verts_pos[1][0], verts_pos[1][1], verts_pos[1][2] );
        LVecBase3f vert3 = LVecBase3f( verts_pos[2][0], verts_pos[2][1], verts_pos[2][2] );
        LVecBase3f vert4 = LVecBase3f(
                                   vert1.get_x() + vert3.get_x() - vert2.get_x(),
                                   vert1.get_y() + vert3.get_y() - vert2.get_y(),
                                   vert1.get_z() + vert3.get_z() - vert2.get_z() );

        result.push_back( vert1 );
        result.push_back( vert2 );
        result.push_back( vert3 );
        result.push_back( vert4 );

        return result;
}

LVecBase3f get_origin( Object &bside, Parser *p )
{

        vector<LVecBase3f> verts = make_vectors( Parser::parse_int_tuple_list_str( p->get_property_value( bside, "plane" ) ) );

        float w = abs( verts[0].get_y() - verts[2].get_y() );
        float l = abs( verts[0].get_x() - verts[1].get_x() );

        LVecBase3f origin( l / 2.0, w / 2.0, verts[0].get_z() );
        return origin;
}

PT( CollisionNode )
make_coll_box( string &name, Object &side_1, Object &side_2, BitMask32 bitmask, bool tangible, Parser *p )
{
        vector<LVecBase3f> verts_1 = make_vectors( Parser::parse_int_tuple_list_str( p->get_property_value( side_1, "plane" ) ) );
        vector<LVecBase3f> verts_2 = make_vectors( Parser::parse_int_tuple_list_str( p->get_property_value( side_2, "plane" ) ) );

        PT( CollisionBox ) box = new CollisionBox( verts_1[0], verts_2[1] );
        box->set_tangible( tangible );
        PT( CollisionNode ) node = new CollisionNode( name );
        node->add_solid( box );
        node->set_collide_mask( bitmask );

        return node;
}

PT( GeomVertexData )
make_geom( vector<LVecBase3f> &verts, CPT( GeomVertexFormat ) format, string &sideId )
{
        PT( GeomVertexData ) vert_data = new GeomVertexData( "sidedata." + sideId, format, Geom::UH_static );
        vert_data->set_num_rows( 4 );

        GeomVertexWriter v_writer( vert_data, InternalName::get_vertex() );
        v_writer.add_data3f( verts[0] );
        v_writer.add_data3f( verts[1] );
        v_writer.add_data3f( verts[2] );
        v_writer.add_data3f( verts[3] );

        return vert_data;
}

PT( GeomNode )
make_geom_pt2( PT( GeomVertexData ) vert_data, string &side_id )
{
        PT( GeomTriangles ) tris = new GeomTriangles( Geom::UH_static );
        tris->set_shade_model( GeomEnums::SM_smooth );
        tris->add_vertices( 1, 0, 2 );
        tris->add_vertices( 2, 0, 3 );
        tris->close_primitive();


        PT( Geom ) geo = new Geom( vert_data );
        geo->add_primitive( tris );

        PT( GeomNode ) side = new GeomNode( "side." + side_id );
        side->add_geom( geo );

        return side;
}

void make_normals( PT( GeomVertexData ) vert_data, vector<LVecBase3f> &verts )
{
        GeomVertexWriter v_writer_norm( vert_data, InternalName::get_normal() );

        LVecBase3f norm = calc_face_normal( verts );

        v_writer_norm.add_data3f( norm[0], norm[1], norm[2] );
        v_writer_norm.add_data3f( norm[0], norm[1], norm[2] );
        v_writer_norm.add_data3f( norm[0], norm[1], norm[2] );
        v_writer_norm.add_data3f( norm[0], norm[1], norm[2] );
}

void make_uvs( PT( GeomVertexData ) vert_data, vector<LVecBase3f> &verts )
{
        vector<LVecBase2f> uvs = calc_uvs( verts, calc_face_normal( verts ) );

        GeomVertexWriter v_writer_tex( vert_data, InternalName::get_texcoord() );
        v_writer_tex.add_data2f( LVecBase2f( 1, 0 ) );
        v_writer_tex.add_data2f( LVecBase2f( 1, 1 ) );
        v_writer_tex.add_data2f( LVecBase2f( 0, 1 ) );
        v_writer_tex.add_data2f( LVecBase2f( 0, 0 ) );
}

void apply_textures( string &mat, string &sideId, Object &sideObj,
                     NodePath &side_np, Parser *p, bool is_disp )
{
        mat += ".png";
        transform( mat.begin(), mat.end(), mat.begin(), tolower );
        PT( Texture ) tex = TexturePool::load_texture( mat );
        PT( TextureStage ) ts = new TextureStage( "ts." + sideId );



        vector<vector<float>> u_data = p->parse_num_array_str( p->get_property_value( sideObj, "uaxis" ) );
        LVecBase3f uaxis( u_data[0][0], u_data[0][1], u_data[0][2] );
        vector<vector<float>> v_data = p->parse_num_array_str( p->get_property_value( sideObj, "vaxis" ) );
        LVecBase3f vaxis( v_data[0][0], v_data[0][1], v_data[0][2] );

        side_np.set_texture( ts, tex, 1 );
        //side_np.set_tex_gen( ts, TexGenAttrib::M_world_position );
        //side_np.set_tex_projector( ts, npProj, side_np );
        //side_np.set_tex_scale( ts, u_data[1][0], v_data[1][0] );
        //side_np.set_tex_scale( ts, u_data[1][0], ( 1 + ( 1 - v_data[1][0] ) ) * 0.9 );
        //side_np.set_tex_offset( ts, u_data[0][3], v_data[0][3] );
        side_np.set_tex_rotate( ts, is_disp ? 0.0 : 90.0 );
        side_np.set_transparency( TransparencyAttrib::M_alpha, 1 );
}

PT( CollisionNode )
make_coll_poly( vector<LVecBase3f> &verts, string &side_id )
{
        PT( CollisionPolygon ) poly = new CollisionPolygon( verts[3], verts[2], verts[1], verts[0] );
        poly->set_tangible( true );

        PT( CollisionNode ) cnode = new CollisionNode( "cnode." + side_id );
        cnode->add_solid( poly );

        if ( get_solid_type( verts ) == ST_floor )
        {
                cnode->set_collide_mask( FLR_BITMASK );
        }
        else
        {
                cnode->set_collide_mask( WALL_BITMASK );
        }

        return cnode;
}

void apply_smooth_shading( NodePath &np )
{
        np.set_attrib( ShadeModelAttrib::make( ShadeModelAttrib::M_smooth ) );
}

Builder::
Builder( const char *output, WindowFramework *window, Parser *p )
{
        cout << "Building..." << endl;

        pWindow = window;
        NodePath render = window->get_render();

        NodePath scene = render.attach_new_node( "scene" );
        //scene.set_scale( 0.05 );

        //PT( EggData ) pData = new EggData;
        //PT( EggVertexPool ) pVPool = new EggVertexPool( "map_vpool" );
        //pData->add_child( pVPool );

        for ( size_t i = 0; i < p->_base_objects.size(); i++ )
        {
                Object obj = p->_base_objects[i];
                if ( obj.name.compare( "world" ) == 0 )
                {
                        string worldId = p->get_property_value( obj, "id" );
                        NodePath world = scene.attach_new_node( "world." + worldId );

                        string sky_name = "";
                        string map_ver = "";
                        string map_title = "";
                        if ( p->has_property( obj, "skyname" ) )
                        {
                                sky_name = p->get_property_value( obj, "skyname" );
                        }
                        if ( p->has_property( obj, "mapversion" ) )
                        {
                                map_ver = p->get_property_value( obj, "mapversion" );
                        }
                        if ( p->has_property( obj, "message" ) )
                        {
                                map_title = p->get_property_value( obj, "message" );
                        }

                        // Store map information in the exported bam file.
                        PT( MapInfo ) mi = new MapInfo( "mapinfo" );
                        mi->set_map_version( map_ver );
                        mi->set_sky_name( sky_name );
                        mi->set_map_title( map_title );
                        world.attach_new_node( mi );

                        for ( size_t j = 0; j < obj.objects.size(); j++ )
                        {
                                Object solObj = obj.objects[j];
                                if ( solObj.name.compare( "solid" ) == 0 )
                                {
                                        string solId = p->get_property_value( solObj, "id" );

                                        vector<Object> sides = p->get_objects_with_name( solObj, "side" );
                                        LVecBase3f origin = get_origin( sides[1], p );

                                        NodePath temp_node = world.attach_new_node( "temp" );
                                        NodePath brush = world.attach_new_node( "brush." + solId );
                                        brush.set_pos( origin );

                                        for ( size_t k = 0; k < sides.size(); k++ )
                                        {
                                                Object sideObj = sides[k];
                                                string side_id = p->get_property_value( sideObj, "id" );

                                                string mat = p->get_property_value( sideObj, "material" );
                                                if ( mat.find( "NODRAW" ) != string::npos || mat.find( "SKYBOX" ) != string::npos )
                                                {
                                                        cout << "Skipping nodraw/skybox polygon." << endl;
                                                        continue;

                                                }
                                                else if ( mat.find( "CLIP" ) != string::npos )
                                                {
                                                        // Clips are just collision boxes.
                                                        vector<Object> sides = p->get_objects_with_name( solObj, "side" );
                                                        PT( CollisionNode ) node = make_coll_box( ( string )"clip_collnode",
                                                                                                  sides[0], sides[1], WALL_BITMASK, true, p );

                                                        NodePath np = temp_node.attach_new_node( node );
                                                        np.wrt_reparent_to( brush );
                                                        break;
                                                }

                                                Quad verts = make_vectors( Parser::parse_int_tuple_list_str( p->get_property_value( sideObj, "plane" ) ) );

                                                //PT( GeomNode ) side;
                                                PT( CollisionNode ) coll_node;

                                                PT( EggData ) data;

                                                bool is_disp = false;

                                                if ( p->has_object_named( sideObj, "dispinfo" ) )
                                                {
                                                        // Displacement!
                                                        is_disp = true;
                                                        data = handle_displacement_side( p, sideObj, side_id, coll_node, verts );
                                                }
                                                else
                                                {
                                                        // It's just a normal brush side.


                                                        //PT( GeomVertexData ) pVertData = MakeGeom( verts, GeomVertexFormat::get_v3n3t2(), sideId );
                                                        //MakeUVs( pVertData, verts );
                                                        //MakeNormals( pVertData, verts );
                                                        //side = MakeGeom_PT2( pVertData, sideId );


                                                        data = make_brush_side( side_id, verts );
                                                        coll_node = make_coll_poly( verts, side_id );
                                                }


                                                NodePath side_np( load_egg_data( data ) );
                                                side_np.wrt_reparent_to( brush );
                                                side_np.attach_new_node( coll_node );
                                                if ( is_facing_up( calc_face_normal( verts ) ) )
                                                {
                                                        cout << "Applying ground bin to side that is facing up." << endl;
                                                        side_np.set_bin( "ground", 18, 1 );
                                                }

                                                apply_smooth_shading( side_np );

                                                apply_textures( mat, side_id, sideObj, side_np, p, is_disp );

                                                if ( is_disp )
                                                {
                                                        cout << "Made displacement, breaking out of solid loop." << endl;
                                                        break;
                                                }

                                        }
                                        temp_node.remove_node();
                                }
                        }
                }
                else if ( obj.name.compare( "entity" ) == 0 )
                {
                        // This is an entity
                        string id = p->get_property_value( obj, "id" );
                        if ( p->get_property_value( obj, "targetname" ).length() > 0 )
                        {
                                id = p->get_property_value( obj, "targetname" );
                        }
                        string classname = p->get_property_value( obj, "classname" );
                        //NodePath entity = scene.attach_new_node("entity." + classname + "." + id);
                        if ( classname.compare( "light_environment" ) == 0 )
                        {
                                // Aha, this is a light entity. Ambient + directional to be exact.
                                vector<int> dir_angle = Parser::parse_num_list_str( p->get_property_value( obj, "angles" ) );
                                vector<int> dir_color = Parser::parse_num_list_str( p->get_property_value( obj, "_light" ) );

                                vector<int> amb_color = Parser::parse_num_list_str( p->get_property_value( obj, "_ambient" ) );

                                int sun_rise = stoi( p->get_property_value( obj, "SunSpreadAngle" ) );
                                int film_size = stoi( p->get_property_value( obj, "_lightscaleHDR" ) );
                                int shadow_size = stoi( p->get_property_value( obj, "_AmbientScaleHDR" ) );

                                PT( AmbientLight ) amblight = new AmbientLight( id + "-AmbientLight" );
                                amblight->set_color( LColor( amb_color[0] / 255.0, amb_color[1] / 255.0, amb_color[2] / 255.0, 1.0 ) );

                                PT( DirectionalLight ) dirlight = new DirectionalLight( id + "-DirectionalLight" );
                                dirlight->set_color( LColor( dir_color[0] / 255.0, dir_color[1] / 255.0, dir_color[2] / 255.0, 1.0 ) );
                                dirlight->get_lens()->set_film_size( film_size, film_size );
                                dirlight->get_lens()->set_near_far( 1, 10000 );

                                NodePath dirnp = scene.attach_new_node( dirlight );
                                dirnp.set_hpr( LVector3( dir_angle[1], dir_angle[0], dir_angle[2] ) );
                                dirnp.set_z( sun_rise / scene_scale );
                                dirnp.set_scale( 1.0 / scene_scale );

                                scene.attach_new_node( amblight );
                        }
                        else if ( classname.find( "trigger" ) != string::npos )
                        {
                                // This is a trigger entity brush.
                                Object solObj = p->get_object_with_name( obj, "solid" );
                                vector<Object> sides = p->get_objects_with_name( solObj, "side" );
                                PT( CollisionNode ) node = make_coll_box( p->get_property_value( obj, "targetname" ) + ".trigger",
                                                                          sides[0], sides[1], EVT_BITMASK, true, p );

                                scene.attach_new_node( node );
                        }
                        else if ( classname.find( "func_door" ) != string::npos )
                        {
                                Object solObj = p->get_object_with_name( obj, "solid" );
                                vector<Object> sides = p->get_objects_with_name( solObj, "side" );
                                LVecBase3f origin = get_origin( sides[1], p );
                                string door_name = p->get_property_value( solObj, "targetname" );
                                if ( door_name.length() == 0 )
                                {
                                        door_name = "door";
                                }
                                NodePath door_np = scene.attach_new_node( "door_origin" );
                                door_np.set_pos( origin );
                                NodePath temp_node = scene.attach_new_node( "temp" );
                                for ( size_t i = 0; i < sides.size(); i++ )
                                {
                                        Object sideObj = sides[i];
                                        string side_id = p->get_property_value( sideObj, "id" );
                                        vector<LVecBase3f> verts = make_vectors( Parser::parse_int_tuple_list_str( p->get_property_value( sideObj, "plane" ) ) );

                                        PT( EggData ) pData = make_brush_side( side_id, verts );

                                        NodePath side_np( load_egg_data( pData ) );
                                        side_np.wrt_reparent_to( door_np );
                                        apply_smooth_shading( side_np );

                                        if ( i > 1 )
                                        {
                                                PT( CollisionNode ) cnode = make_coll_poly( verts, side_id );
                                                side_np.attach_new_node( cnode );
                                        }


                                        string mat = p->get_property_value( sideObj, "material" );
                                        apply_textures( mat, side_id, sideObj, side_np, p, false );

                                }

                                PT( Door ) door = new Door( door_name );
                                vector<int> movedir = Parser::parse_num_list_str( p->get_property_value( obj, "movedir" ) );
                                door->set_move_dir( LPoint3f( movedir[0], movedir[1], movedir[2] ) );
                                door->set_speed( stof( p->get_property_value( obj, "speed" ) ) );
                                door->set_spawn_pos( ( Door::SpawnPos )stoi( p->get_property_value( obj, "spawnpos" ) ) );
                                door->set_wait( stof( p->get_property_value( obj, "wait" ) ) );
                                door->set_locked_sentence( p->get_property_value( obj, "locked_sentence" ) );
                                door->set_unlocked_sentence( p->get_property_value( obj, "unlocked_sentence" ) );

                                scene.attach_new_node( door );
                                PT( CollisionNode ) cbox = make_coll_box( door_name + ".trigger", sides[0], sides[1], EVT_BITMASK, false, p );
                                NodePath cboxnp = temp_node.attach_new_node( cbox );
                                cboxnp.wrt_reparent_to( door_np );

                                temp_node.remove_node();
                        }
                        else if ( classname.compare( "func_occluder" ) == 0 )
                        {
                                // Occluder!
                                Object solObj = p->get_object_with_name( obj, "solid" );
                                vector<float> side_lengths;
                                vector<Object> sides = p->get_objects_with_name( solObj, "side" );
                                for ( size_t i = 0; i < sides.size(); i++ )
                                {
                                        Object sideObj = sides[i];
                                        vector<LVecBase3f> verts = make_vectors( Parser::parse_int_tuple_list_str( p->get_property_value( sideObj, "plane" ) ) );

                                        // Occluders have to be a flat plane, and hammer only makes blocks.
                                        // What we're going to do is find the longest side of the block and use that for the occluder plane.
                                        LVecBase3f norm = calc_face_normal( verts );

                                        if ( is_facing_left( norm ) || is_facing_right( norm ) )
                                        {
                                                // Calculate the diagonal length of the side.
                                                float w = abs( verts[0].get_z() - verts[2].get_z() );
                                                float l = abs( verts[0].get_y() - verts[1].get_y() );
                                                float d = sqrtf( ( w * w ) + ( l * l ) );
                                                side_lengths.push_back( d );
                                        }
                                        else if ( is_facing_forward( norm ) || is_facing_backward( norm ) )
                                        {
                                                // Calculate the diagonal length of the side.
                                                float w = abs( verts[0].get_z() - verts[2].get_z() );
                                                float l = abs( verts[0].get_x() - verts[1].get_x() );
                                                float d = sqrtf( ( w * w ) + ( l * l ) );
                                                side_lengths.push_back( d );
                                        }
                                        else if ( is_facing_up( norm ) || is_facing_down( norm ) )
                                        {
                                                // Calculate the diagonal length of the side.
                                                float w = abs( verts[0].get_x() - verts[1].get_x() );
                                                float l = abs( verts[0].get_y() - verts[2].get_y() );
                                                float d = sqrtf( ( w * w ) + ( l * l ) );
                                                side_lengths.push_back( d );
                                        }
                                }

                                int longest_side_index = distance( side_lengths.begin(), max_element( side_lengths.begin(), side_lengths.end() ) );

                                Object longest_side = sides[longest_side_index];
                                string sideId = p->get_property_value( longest_side, "id" );
                                vector<LVecBase3f> verts = make_vectors( Parser::parse_int_tuple_list_str( p->get_property_value( longest_side, "plane" ) ) );
                                PT( OccluderNode ) occ = new OccluderNode( "occluder." + id );
                                occ->set_double_sided( true );
                                occ->set_vertices( verts[0], verts[1], verts[2], verts[3] );

                                scene.attach_new_node( occ );
                        }
                        else if ( classname.compare( "env_fog_controller" ) == 0 )
                        {
                                cout << "Making fog..." << endl;

                                // Fog

                                PT( Fog ) pFog = new Fog( id );

                                vector<float> color = Parser::parse_float_list_str( p->get_property_value( obj, "fogcolor" ) );
                                pFog->set_color( color[0] / 255.0, color[1] / 255.0, color[2] / 255.0 );

                                float flStart = stof( p->get_property_value( obj, "fogstart" ) );
                                float flEnd = stof( p->get_property_value( obj, "fogend" ) );
                                pFog->set_exp_density( flStart / flEnd );

                                NodePath fog_np = scene.attach_new_node( pFog );

                                cout << "Made env_fog_controller (" << fog_np.get_name() << "):\n"
                                     << "\t" << pFog->get_exp_density() << pFog->get_color() << endl;

                                //float flFarZ = stof( p->get_property_value( obj, "farz" ) );
                        }
                        else if ( classname.compare( "light" ) == 0 )
                        {
                                // Point light entity
                                PT( PointLight ) pLight = new PointLight( "pointlight." + id );

                                float flQuad = stof( p->get_property_value( obj, "_quadratic_attn" ) );
                                float flConst = stof( p->get_property_value( obj, "_constant_attn" ) );
                                float flLinear = stof( p->get_property_value( obj, "_linear_attn" ) );
                                //pLight->set_attenuation( LVecBase3f( flConst, flLinear, flQuad ) );

                                vector<float> color = Parser::parse_float_list_str( p->get_property_value( obj, "_light" ) );
                                pLight->set_color( LColor( color[0] / 255.0, color[1] / 255.0, color[2] / 255.0, 1.0 ) );

                                //pLight->set_shadow_caster(true, 1024, 1024);

                                vector<float> pos = Parser::parse_float_list_str( p->get_property_value( obj, "origin" ) );

                                NodePath npLight = scene.attach_new_node( pLight );
                                npLight.set_pos( render, pos[0], pos[1], pos[2] );
                        }
                        else if ( classname.compare( "light_spot" ) == 0 )
                        {
                                // Spot light entity
                                PT( Spotlight ) pLight = new Spotlight( "spotlight." + id );

                                float flQuad = stof( p->get_property_value( obj, "_quadratic_attn" ) );
                                float flConst = stof( p->get_property_value( obj, "_constant_attn" ) );
                                float flLinear = stof( p->get_property_value( obj, "_linear_attn" ) );
                                //pLight->set_attenuation( LVecBase3f( flConst, flLinear, flQuad ) );

                                vector<float> color = Parser::parse_float_list_str( p->get_property_value( obj, "_light" ) );
                                pLight->set_color( LColor( color[0] / 255.0, color[1] / 255.0, color[2] / 255.0, 1.0 ) );

                                float flExp = stof( p->get_property_value( obj, "_exponent" ) );
                                pLight->set_exponent( flExp );

                                //pLight->set_shadow_caster(true, 1024, 1024);

                                vector<float> pos = Parser::parse_float_list_str( p->get_property_value( obj, "origin" ) );
                                vector<float> hpr = Parser::parse_float_list_str( p->get_property_value( obj, "angles" ) );

                                NodePath npLight = scene.attach_new_node( pLight );
                                npLight.set_pos( render, pos[0], pos[1], pos[2] );
                                npLight.set_hpr( render, hpr[1], hpr[0], hpr[2] );
                        }
                        else if ( classname.compare( "env_sun" ) == 0 )
                        {
                                // Sun visual model thingy.

                                PT( Sun ) sun = new Sun( "env_sun." + id );
                                vector<float> col = Parser::parse_float_list_str( p->get_property_value( obj, "rendercolor" ) );
                                sun->set_sun_color( LColor( col[0] / 255.0, col[1] / 255.0, col[2] / 255.0, 1.0 ) );

                                vector<float> pos = Parser::parse_float_list_str( p->get_property_value( obj, "origin" ) );

                                float scale = stof( p->get_property_value( obj, "size" ) );

                                NodePath sun_np = scene.attach_new_node( sun );
                                //sun_np.set_bin("background", 1);
                                sun_np.set_pos( render, LPoint3( pos[0], pos[1], pos[2] ) );
                                sun_np.set_color( col[0] / 255.0, col[1] / 255.0, col[2] / 255.0 );
                                sun_np.set_scale( scale );
                                sun_np.set_compass();
                                sun_np.set_billboard_point_world();
                                sun_np.set_transparency( TransparencyAttrib::M_dual, 1 );
                                sun_np.set_light_off();
                                sun_np.set_fog_off();
                                sun_np.set_material_off();
                        }
                        else if ( classname.compare( "point_devshot_camera" ) == 0 )
                        {
                                // Devshot camera, used for viewing angles of the map (maybe for spectate or something)

                                string cam_name = p->get_property_value( obj, "cameraname" );

                                PT( DevshotCamera ) cam = new DevshotCamera( cam_name );

                                PN_stdfloat fov = stof( p->get_property_value( obj, "FOV" ) );
                                cam->set_fov( fov );

                                vector<float> pos = Parser::parse_float_list_str( p->get_property_value( obj, "origin" ) );
                                vector<float> hpr = Parser::parse_float_list_str( p->get_property_value( obj, "angles" ) );

                                NodePath cam_np = scene.attach_new_node( cam );
                                cam_np.set_pos( render, LPoint3( pos[0], pos[1], pos[2] ) );
                                cam_np.set_hpr( render, LVector3( hpr[1], hpr[0], hpr[2] ) );

                                cout << "Made point_devshot_camera (" << cam->get_name() << "):\n"
                                     << "\t" << fov << " " << cam_np.get_pos() << " " << cam_np.get_hpr() << endl;

                                //} else if (classname.compare("entity_spawn_point") == 0) {
                                //  string point_name = p->get_property_value(obj, "targetname");
                                //  string spm_name = p->get_property_value(obj, "spawn_manager_name");
                                //  vector<float> pos = Parser::parse_float_list_str(p->get_property_value(obj, "origin"));

                        }
                        else if ( classname.compare( "info_null" ) == 0 )
                        {
                                // This entity is used to just associate a name with a position in space.
                                // Very useful.
                                // It's just a PandaNode with a position applied to it.

                                vector<float> pos = Parser::parse_float_list_str( p->get_property_value( obj, "origin" ) );

                                PT( PandaNode ) null_node = new PandaNode( id );

                                NodePath null_np = scene.attach_new_node( null_node );
                                null_np.set_pos( render, pos[0], pos[1], pos[2] );

                                cout << "Made info_null, which is just a PandaNode (" << null_np.get_name() << "):\n"
                                     << "\t" << null_np.get_pos( render ) << endl;
                        }
                }
        }

        scene.set_scale( scene_scale );
        scene.ls();

        cout << "Writing to " << output << "..." << endl;
        scene.write_bam_file( output );
}