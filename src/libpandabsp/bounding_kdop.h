/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file bounding_kdop.h
 * @author Brian Lach
 * @date January 17, 2019
 */

#ifndef BOUNDING_KDOP_H
#define BOUNDING_KDOP_H

#include <pandabase.h>

#include <finiteBoundingVolume.h>
#include <frustum.h>
#include <plane.h>

#include <coordinateSystem.h>
#include <boundingSphere.h>
#include <boundingBox.h>
#include <boundingHexahedron.h>

#include "config_bsp.h"


/**
 * This defines a bounding convex volume.  It may represent any enclosing convex volume,
 * including simple boxes.  However, if all you want is an axis-aligned
 * bounding box, you may be better off with the simpler BoundingBox class.
 */
class EXPCL_PANDABSP BoundingKDOP : public FiniteBoundingVolume
{
public:
        INLINE BoundingKDOP() :
                FiniteBoundingVolume()
        {
        }

        INLINE BoundingKDOP( const BoundingKDOP &other ) :
                FiniteBoundingVolume( other ),
                _planes( other._planes ),
                _points( other._points ),
                _centroid( other._centroid )
        {
        }

        BoundingKDOP( const pvector<LPlane> &planes );

public:
        ALLOC_DELETED_CHAIN( BoundingKDOP );
        virtual BoundingVolume *make_copy() const;

        virtual LPoint3 get_min() const;
        virtual LPoint3 get_max() const;

        virtual LPoint3 get_approx_center() const;
        virtual void xform( const LMatrix4 &mat );

        virtual void output( std::ostream &out ) const;
        virtual void write( std::ostream &out, int indent_level = 0 ) const;

PUBLISHED:
        INLINE size_t get_num_points() const
        {
                return _points.size();
        }
        INLINE LPoint3 get_point( int n ) const
        {
                nassertr( n >= 0 && n < _points.size(), LPoint3::zero() );
                return _points[n];
        }
        INLINE size_t get_num_planes() const
        {
                return _planes.size();
        }
        INLINE LPlane get_plane( int n ) const
        {
                nassertr( n >= 0 && n < _planes.size(), LPlane() );
                return _planes[n];
        }

protected:
        virtual bool extend_other( BoundingVolume *other ) const;

        virtual bool around_other( BoundingVolume *other,
                                   const BoundingVolume **first,
                                   const BoundingVolume **last ) const;
        virtual int contains_other( const BoundingVolume *other ) const;

        virtual int contains_point( const LPoint3 &point ) const;
        virtual int contains_lineseg( const LPoint3 &a, const LPoint3 &b ) const;
        virtual int contains_sphere( const BoundingSphere *sphere ) const;
        virtual int contains_box( const BoundingBox *box ) const;
        virtual int contains_hexahedron( const BoundingHexahedron *hexahedron ) const;

private:
        void set_points();
        void set_centroid();

private:
        pvector<LPoint3> _points;
        pvector<LPlane> _planes;
        LPoint3 _centroid;

public:
        static TypeHandle get_class_type()
        {
                return _type_handle;
        }
        static void init_type()
        {
                FiniteBoundingVolume::init_type();
                register_type( _type_handle, "BoundingKDOP",
                               FiniteBoundingVolume::get_class_type() );
        }
        virtual TypeHandle get_type() const
        {
                return get_class_type();
        }
        virtual TypeHandle force_init_type() { init_type(); return get_class_type(); }

private:
        static TypeHandle _type_handle;

        friend class BoundingSphere;
        friend class BoundingBox;
};

#endif // BOUNDING_KDOP_H
