#ifndef BSP_RAYTRACE_H
#define BSP_RAYTRACE_H

#include "config_bsp.h"

#include <aa_luse.h>
#include <bitMask.h>
#include <referenceCount.h>
#include <transformState.h>
#include <pandaNode.h>
#include <cullTraverser.h>
#include <cullTraverserData.h>

NotifyCategoryDeclNoExport(raytrace);

// embree forward decls
struct RTCDeviceTy;
typedef struct RTCDeviceTy* RTCDevice;
struct RTCSceneTy;
typedef struct RTCSceneTy* RTCScene;
struct RTCGeometryTy;
typedef struct RTCGeometryTy* RTCGeometry;

class EXPCL_PANDABSP RayTrace
{
PUBLISHED:
        static void initialize();
        static void destruct();

public:
        INLINE static RTCDevice get_device()
        {
                return _device;
        }

private:
        static bool _initialized;
        static RTCDevice _device;
};

class EXPCL_PANDABSP RayTraceHitResult
{
public:
        LVector3 hit_normal;
        LVector2 hit_uv;
        unsigned int prim_id;
        unsigned int geom_id;
        float hit_fraction;
        bool hit;

PUBLISHED:
        INLINE RayTraceHitResult()
        {
                prim_id = 0;
                geom_id = 0;
                hit_fraction = 0;
                hit = false;
        }

        INLINE bool has_hit() const
        {
                return hit;
        }
        INLINE LVector3 get_hit_normal() const
        {
                return hit_normal;
        }
        INLINE LVector2 get_uv() const
        {
                return hit_uv;
        }
        INLINE unsigned int get_prim_id() const
        {
                return prim_id;
        }
        INLINE unsigned int get_geom_id() const
        {
                return geom_id;
        }
        INLINE float get_hit_fraction() const
        {
                return hit_fraction;
        }
};

class RayTraceGeometry;

class EXPCL_PANDABSP RayTraceScene : public ReferenceCount
{
PUBLISHED:
        RayTraceScene();
        ~RayTraceScene();

        void add_geometry( RayTraceGeometry *geom );
        void remove_geometry( RayTraceGeometry *geom );
        void remove_all();

        INLINE RayTraceHitResult trace_line( const LPoint3 &start, const LPoint3 &end, const BitMask32 &mask )
        {
                LPoint3 delta = end - start;
                return trace_ray( start, delta.normalized(), delta.length(), mask );
        }
        RayTraceHitResult trace_ray( const LPoint3 &origin, const LVector3 &direction,
                float distance, const BitMask32 &mask );

        void update();

private:
        RTCScene _scene;
        bool _scene_needs_rebuild;

        pvector<RayTraceGeometry *> _geoms;

        friend class RayTraceGeometry;
};

class EXPCL_PANDABSP RayTraceGeometry : public PandaNode
{
        TypeDecl( RayTraceGeometry, PandaNode );

PUBLISHED:
        INLINE RayTraceGeometry( const std::string &name = "" ) :
                PandaNode( name ),
                _geometry( nullptr ),
                _geom_id( 0 ),
                _rtscene( nullptr ),
                _last_trans( nullptr )
        {
        }
        virtual ~RayTraceGeometry();

        INLINE void set_mask( const BitMask32 &mask )
        {
                set_mask( mask.get_word() );
        }
        void set_mask( unsigned int mask );

        virtual void build() = 0;

public:
        RayTraceGeometry( int type, const std::string &name = "" );

        INLINE RTCGeometry get_geometry() const
        {
                return _geometry;
        }

        void update_rtc_transform( const TransformState *ts );

protected:
        RTCGeometry _geometry;
        unsigned int _geom_id;
        RayTraceScene *_rtscene;

        CPT(TransformState) _last_trans;

        friend class RayTraceScene;
};

class EXPCL_PANDABSP RayTraceTriangleMesh : public RayTraceGeometry
{
        TypeDecl( RayTraceTriangleMesh, RayTraceGeometry );

PUBLISHED:
        RayTraceTriangleMesh( const std::string &name = "" );

        void add_triangle( const LPoint3 &p1, const LPoint3 &p2, const LPoint3 &p3 );
        void add_triangles_from_geom( const Geom *geom, const TransformState *ts = nullptr );

        virtual void build();

private:
        struct Triangle
        {
                int v1, v2, v3;
        };
        pvector<LPoint3> _verts;
        pvector<Triangle> _tris;
};

#endif // BSP_RAYTRACE_H