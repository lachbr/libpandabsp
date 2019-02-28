#include "raytrace.h"

#include <geomVertexReader.h>

#include <embree3/rtcore.h>

NotifyCategoryDef( raytrace, "" );

bool RayTrace::_initialized = false;
RTCDevice RayTrace::_device = nullptr;

void RayTrace::initialize()
{
        if ( _initialized )
                return;

        _device = rtcNewDevice( "" );

        _initialized = true;
}

void RayTrace::destruct()
{
        _initialized = false;
        if ( _device )
                rtcReleaseDevice( _device );
        _device = nullptr;
}

//==================================================================//

TypeDef( RayTraceGeometry );

RayTraceGeometry::RayTraceGeometry( int type, const std::string &name ) :
        PandaNode( name )
{
        nassertv( RayTrace::get_device() != nullptr );
        _geometry = rtcNewGeometry( RayTrace::get_device(), (RTCGeometryType)type );
        // All bits on by default
        rtcSetGeometryMask( _geometry, BitMask32::all_on().get_word() );
        _geom_id = 0;
        _rtscene = nullptr;
        _last_trans = nullptr;

        set_cull_callback();


        raytrace_cat.debug()
                << "Made new RayTraceGeometry, type " << type << "\n";
}

RayTraceGeometry::~RayTraceGeometry()
{
        if ( _rtscene )
                _rtscene->remove_geometry( this );
        if ( _geometry )
                rtcReleaseGeometry( _geometry );
        _geometry = nullptr;
}

void RayTraceGeometry::update_rtc_transform( const TransformState *ts )
{
        if ( ts != _last_trans )
        {
                _last_trans = ts;

                const LMatrix4 &mat = _last_trans->get_mat();

                rtcSetGeometryTransform( _geometry, 0, RTC_FORMAT_FLOAT4X4_ROW_MAJOR, mat.get_data() );
                rtcCommitGeometry( _geometry );
                if ( _rtscene )
                        _rtscene->_scene_needs_rebuild = true;

                raytrace_cat.debug()
                        << "Updated geometry transform\n";
        }
}

void RayTraceGeometry::set_mask( unsigned int mask )
{
        nassertv( _geometry != nullptr );
        rtcSetGeometryMask( _geometry, mask );
}

//==================================================================//

TypeDef( RayTraceTriangleMesh );

RayTraceTriangleMesh::RayTraceTriangleMesh( const std::string &name) :
        RayTraceGeometry( RTC_GEOMETRY_TYPE_TRIANGLE, name )
{
}

void RayTraceTriangleMesh::add_triangle( const LPoint3 &p1, const LPoint3 &p2, const LPoint3 &p3 )
{
        nassertv( _geometry != nullptr );
        int idx = (int)_verts.size();
        _verts.push_back( p1 );
        _verts.push_back( p2 );
        _verts.push_back( p3 );
        Triangle tri;
        tri.v1 = idx;
        tri.v2 = idx + 1;
        tri.v3 = idx + 2;
        _tris.push_back( tri );

        raytrace_cat.debug()
                << "Added triangle [" << p1 << ", " << p2 << ", " << p3 << "]\n";
}

void RayTraceTriangleMesh::add_triangles_from_geom( const Geom *geom, const TransformState *ts )
{
        if ( ts == nullptr )
                ts = TransformState::make_identity();

        const LMatrix4 &mat = ts->get_mat();

        PT( Geom ) dgeom = geom->decompose();

        nassertv( dgeom->get_primitive_type() == Geom::PT_polygons );

        const GeomVertexData *vdata = dgeom->get_vertex_data();
        GeomVertexReader reader( vdata, InternalName::get_vertex() );
        
        for ( int i = 0; i < dgeom->get_num_primitives(); i++ )
        {
                const GeomPrimitive *prim = dgeom->get_primitive( i );

                nassertv( prim->get_num_vertices_per_primitive() == 3 );

                for ( int j = 0; j < prim->get_num_primitives(); j++ )
                {
                        int start = prim->get_primitive_start( j );

                        reader.set_row( prim->get_vertex( start ) );
                        LPoint3 p1 = reader.get_data3f();
                        reader.set_row( prim->get_vertex( start + 1 ) );
                        LPoint3 p2 = reader.get_data3f();
                        reader.set_row( prim->get_vertex( start + 2 ) );
                        LPoint3 p3 = reader.get_data3f();

                        add_triangle( mat.xform_point( p1 ), mat.xform_point( p2 ), mat.xform_point( p3 ) );
                }
        }
}

void RayTraceTriangleMesh::build()
{
        LPoint4f *vertices = (LPoint4f *)rtcSetNewGeometryBuffer( _geometry, RTC_BUFFER_TYPE_VERTEX, 0,
                                                                  RTC_FORMAT_FLOAT3, sizeof( LPoint4f ), 
                                                                  _verts.size() );
        raytrace_cat.debug()
                << "build(): vertex buffer: " << vertices << "\n";
        for ( size_t i = 0; i < _verts.size(); i++ )
        {
                vertices[i] = LPoint4f( _verts[i], 0 );
        }

        Triangle *tris = (Triangle *)rtcSetNewGeometryBuffer( _geometry, RTC_BUFFER_TYPE_INDEX, 0,
                                                              RTC_FORMAT_UINT3, sizeof( Triangle ), _tris.size() );
        raytrace_cat.debug()
                << "build(): triangle buffer: " << tris << "\n";
        for ( size_t i = 0; i < _tris.size(); i++ )
        {
                tris[i] = _tris[i];
        }

        rtcCommitGeometry( _geometry );

        raytrace_cat.debug()
                << "Built triangle mesh to embree\n";
}

//==================================================================//

RayTraceScene::RayTraceScene()
{
        nassertv( RayTrace::get_device() != nullptr );
        _scene = rtcNewScene( RayTrace::get_device() );
        raytrace_cat.debug()
                << "Made new raytrace scene\n";
}

RayTraceScene::~RayTraceScene()
{
        if ( _scene )
                rtcReleaseScene( _scene );
}

void RayTraceScene::add_geometry( RayTraceGeometry *geom )
{
        unsigned int geom_id = rtcAttachGeometry( _scene, geom->get_geometry() );
        geom->_geom_id = geom_id;
        geom->_rtscene = this;
        _geoms.push_back( geom );
        raytrace_cat.debug()
                << "Attached geometry " << geom_id << "\n";
}

void RayTraceScene::remove_geometry( RayTraceGeometry *geom )
{
        rtcDetachGeometry( _scene, geom->_geom_id );
        geom->_geom_id = 0;
        geom->_rtscene = nullptr;
        _geoms.erase( std::find( _geoms.begin(), _geoms.end(), geom ) );
}

void RayTraceScene::remove_all()
{
        for ( size_t i = 0; i < _geoms.size(); i++ )
        {
                RayTraceGeometry *geom = _geoms[i];
                rtcDetachGeometry( _scene, geom->_geom_id );
                geom->_geom_id = 0;
                geom->_rtscene = nullptr;
        }

        _geoms.clear();
}

void RayTraceScene::update()
{
        nassertv( _scene != nullptr );

        size_t num_geoms = _geoms.size();
        for ( size_t i = 0; i < num_geoms; i++ )
        {
                RayTraceGeometry *geom = _geoms[i];
                geom->update_rtc_transform( NodePath( geom ).get_net_transform() );
        }

        if ( _scene_needs_rebuild )
        {
                raytrace_cat.debug()
                        << "Committing scene\n";
                rtcCommitScene( _scene );
                _scene_needs_rebuild = false;
        }
}

RayTraceHitResult RayTraceScene::trace_ray( const LPoint3 &start, const LVector3 &dir,
        float distance, const BitMask32 &mask )
{
        RayTraceHitResult result;

        RTCIntersectContext ctx;
        rtcInitIntersectContext( &ctx );

        RTCRay ray;
        ray.mask = mask.get_word();
        ray.org_x = start[0];
        ray.org_y = start[1];
        ray.org_z = start[2];
        ray.dir_x = dir[0];
        ray.dir_y = dir[1];
        ray.dir_z = dir[2];
        ray.tnear = 0;
        ray.tfar = distance;
        RTCRayHit rhit;
        rhit.ray = ray;

        // Trace a ray
        rtcIntersect1( _scene, &ctx, &rhit );

        // Store the results
        result.hit_fraction = rhit.ray.tfar / distance;
        result.hit_normal = LVector3( rhit.hit.Ng_x,
                rhit.hit.Ng_y, rhit.hit.Ng_z );
        result.hit_uv = LVector2( rhit.hit.u, rhit.hit.v );
        result.geom_id = rhit.hit.geomID;
        result.prim_id = rhit.hit.primID;
        // If the ray/line didn't trace all the way to the end,
        // we have a hit.
        result.hit = result.hit_fraction < 1.0f;

        raytrace_cat.debug()
                << "Traced ray from " << start << " direction " << dir
                << "\ndistance " << distance << " mask " << mask.get_word() << "\n";
        raytrace_cat.debug()
                << "\tHas hit? " << result.hit << "\n"
                << "\tHit fraction " << result.hit_fraction << "\n"
                << "\tHit Normal " << result.hit_normal << "\n"
                << "\tHit UV " << result.hit_uv << "\n"
                << "\tGeom ID " << result.geom_id << "\n"
                << "\tPrim ID " << result.prim_id << "\n";

        return result;
}

//==================================================================//