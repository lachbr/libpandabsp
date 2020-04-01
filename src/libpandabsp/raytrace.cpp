#include "raytrace.h"

#include <geomVertexReader.h>

#include <embree3/rtcore.h>

NotifyCategoryDef( raytrace, "" );

static const ALIGN_16BYTE int32_t Four_NegativeOnes_NonSIMD[4] = { -1, -1, -1, -1 };

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

IMPLEMENT_CLASS( RayTraceGeometry );

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
        _mask = mask;
}

void RayTraceGeometry::set_build_quality( int quality )
{
        rtcSetGeometryBuildQuality( _geometry, (RTCBuildQuality)quality );
};

//==================================================================//

IMPLEMENT_CLASS( RayTraceTriangleMesh );

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
        RTCError err = rtcGetDeviceError( RayTrace::get_device() );
        raytrace_cat.debug()
                << "add_geometry: rtcError: " << err << "\n";
        geom->_geom_id = geom_id;
        geom->_rtscene = this;
        _geoms[geom_id] = geom;
        raytrace_cat.debug()
                << "Attached geometry " << geom_id << "\n";
        _scene_needs_rebuild = true;
}

void RayTraceScene::remove_geometry( RayTraceGeometry *geom )
{
        rtcDetachGeometry( _scene, geom->_geom_id );
        geom->_geom_id = 0;
        geom->_rtscene = nullptr;
        _geoms.remove( geom->_geom_id );
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

void RayTraceScene::set_build_quality( int quality )
{
        rtcSetSceneBuildQuality( _scene, (RTCBuildQuality)quality );
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
                raytrace_cat.info()
                        << "Committing scene\n";
                rtcCommitScene( _scene );
                _scene_needs_rebuild = false;
        }
}

RayTraceHitResult RayTraceScene::trace_ray( const LPoint3 &start, const LVector3 &dir,
        float distance, const BitMask32 &mask )
{

        RayTraceHitResult result;

        //nassertr( RayTrace::get_device() != nullptr, result );
        //nassertr( _scene != nullptr, result );

        RTCIntersectContext ctx;
        rtcInitIntersectContext( &ctx );
        ctx.flags = RTC_INTERSECT_CONTEXT_FLAG_COHERENT;

        ALIGN_16BYTE RTCRay ray;
        ray.mask = mask.get_word();
        ray.org_x = start[0];
        ray.org_y = start[1];
        ray.org_z = start[2];
        ray.dir_x = dir[0];
        ray.dir_y = dir[1];
        ray.dir_z = dir[2];
        ray.tnear = 0;
        ray.tfar = distance;
        ray.flags = 0;
        ALIGN_16BYTE RTCRayHit rhit;
        rhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        rhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
        rhit.ray = ray;

        //raytrace_cat.debug()
        //        << "Tracing ray from " << start << ", direction " << dir << ", distance " << distance << ", mask " << mask << std::endl;

        // Trace a ray
        rtcIntersect1( _scene, &ctx, &rhit );

        //RTCError err = rtcGetDeviceError( RayTrace::get_device() );
        //raytrace_cat.debug()
        //        << "trace_ray: rtcError: " << err << "\n";

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

        //raytrace_cat.debug()
        //        << "Traced ray from " << start << " direction " << dir
        //        << "\ndistance " << distance << " mask " << mask.get_word() << "\n";
        //raytrace_cat.debug()
        //        << "\tHas hit? " << result.hit << "\n"
        //        << "\tHit fraction " << result.hit_fraction << "\n"
        //        << "\tHit Normal " << result.hit_normal << "\n"
        //        << "\tHit UV " << result.hit_uv << "\n"
        //        << "\tGeom ID " << result.geom_id << "\n"
        //        << "\tPrim ID " << result.prim_id << "\n";

        return result;
}

void RayTraceScene::trace_four_rays( const FourVectors &start, const FourVectors &direction,
        const fltx4 &distance, const u32x4 &mask, RayTraceHitResult4 *res )
{

        RTCIntersectContext ctx;
        rtcInitIntersectContext( &ctx ); 
        //ctx.flags = RTC_INTERSECT_CONTEXT_FLAG_COHERENT;

        ALIGN_16BYTE RTCRayHit4 rhit4;
        StoreAlignedUIntSIMD( rhit4.hit.geomID, Four_NegativeOnes );
        StoreAlignedUIntSIMD( rhit4.hit.instID[0], Four_NegativeOnes );
        StoreAlignedSIMD( rhit4.ray.org_x, start.x );
        StoreAlignedSIMD( rhit4.ray.org_y, start.y );
        StoreAlignedSIMD( rhit4.ray.org_z, start.z );
        StoreAlignedSIMD( rhit4.ray.dir_x, direction.x );
        StoreAlignedSIMD( rhit4.ray.dir_y, direction.y );
        StoreAlignedSIMD( rhit4.ray.dir_z, direction.z );
        StoreAlignedUIntSIMD( rhit4.ray.mask, mask );
        StoreAlignedSIMD( rhit4.ray.tnear, Four_Zeros );
        StoreAlignedSIMD( rhit4.ray.tfar, distance );
        StoreAlignedUIntSIMD( rhit4.ray.flags, Four_Zeros );
        
        rtcIntersect4( Four_NegativeOnes_NonSIMD, _scene, &ctx, &rhit4 );

        res->geom_id = LoadAlignedIntSIMD( rhit4.hit.geomID );

        fltx4 factor = ReciprocalSIMD( distance );
        res->hit_fraction = MulSIMD( LoadAlignedSIMD( rhit4.ray.tfar ), factor );
        //res->hit = CmpLtSIMD( res->hit_fraction, Four_Ones );
}

//==================================================================//