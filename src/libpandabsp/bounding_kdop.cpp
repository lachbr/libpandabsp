/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file bounding_kdop.cpp
 * @author Brian Lach
 * @date January 17, 2019
 */

#include "bounding_kdop.h"

#include <boundingSphere.h>
#include <boundingBox.h>
#include <boundingPlane.h>
#include <boundingHexahedron.h>

TypeHandle BoundingKDOP::_type_handle;

BoundingKDOP::BoundingKDOP( const pvector<LPlane> &planes )
{
        _planes = planes;

        _flags = 0;
        set_centroid();
        set_points();
}

BoundingVolume *BoundingKDOP::make_copy() const
{
        return new BoundingKDOP( *this );
}

bool BoundingKDOP::extend_other( BoundingVolume *other ) const
{
        return false;//return other->extend_by_hexahedron( this );
}

bool BoundingKDOP::around_other( BoundingVolume *other,
              const BoundingVolume **first,
              const BoundingVolume **last ) const
{
        return false;
}

int BoundingKDOP::contains_other( const BoundingVolume *other ) const
{
        return false;
}

LPoint3 BoundingKDOP::get_min() const
{
        return LPoint3( -10, -10, -10 );
}

LPoint3 BoundingKDOP::get_max() const
{
        return LPoint3( 10, 10, 10 );
}

LPoint3 BoundingKDOP::get_approx_center() const
{
        return LPoint3::zero();
}

void BoundingKDOP::output( std::ostream &out ) const
{
}

void BoundingKDOP::write( std::ostream &out, int indent_level ) const
{
}

void BoundingKDOP::xform( const LMatrix4 &mat )
{
        if ( !is_empty() && !is_infinite() )
        {
                for ( size_t i = 0; i < _planes.size(); i++ )
                {
                        _planes[i] = _planes[i] * mat;
                }
                set_centroid();
                set_points();
        }
}

int BoundingKDOP::contains_point( const LPoint3 &point ) const
{
        if ( is_empty() )
        {
                return IF_no_intersection;

        }
        else if ( is_infinite() )
        {
                return IF_possible | IF_some | IF_all;

        }
        else
        {
                // The hexahedron contains the point iff the point is behind all of the
                // planes.
                for ( size_t i = 0; i < _planes.size(); i++ )
                {
                        const LPlane &p = _planes[i];
                        if ( p.dist_to_plane( point ) > 0.0f )
                        {
                                return IF_no_intersection;
                        }
                }
                return IF_possible | IF_some | IF_all;
        }
}

int BoundingKDOP::contains_lineseg( const LPoint3 &a, const LPoint3 &b ) const
{
        if ( is_empty() )
        {
                return IF_no_intersection;

        }
        else if ( is_infinite() )
        {
                return IF_possible | IF_some | IF_all;

        }
        else
        {
                // The hexahedron does not contains the line segment if both points are in
                // front of any one plane.
                for ( size_t i = 0; i < _planes.size(); i++ )
                {
                        const LPlane &p = _planes[i];
                        if ( p.dist_to_plane( a ) > 0.0f ||
                             p.dist_to_plane( b ) > 0.0f )
                        {
                                return IF_no_intersection;
                        }
                }

                // If there is no plane that both points are in front of, the hexahedron
                // may or may not contain the line segment.  For the moment, we won't
                // bother to check that more thoroughly, though.
                return IF_possible;
        }
}

int BoundingKDOP::contains_sphere( const BoundingSphere *sphere ) const
{
        nassertr( !is_empty(), 0 );

        // The hexahedron contains the sphere iff the sphere is at least partly
        // behind all of the planes.
        const LPoint3 &center = sphere->get_center();
        PN_stdfloat radius = sphere->get_radius();

        int result = IF_possible | IF_some | IF_all;

        for ( size_t i = 0; i < _planes.size(); i++ )
        {
                const LPlane &p = _planes[i];
                PN_stdfloat dist = p.dist_to_plane( center );

                if ( dist > radius )
                {
                        // The sphere is completely in front of this plane; it's thus completely
                        // outside of the hexahedron.
                        return IF_no_intersection;

                }
                else if ( dist > -radius )
                {
                        // The sphere is not completely behind this plane, but some of it is.
                        result &= ~IF_all;
                }
        }

        return result;
}

int BoundingKDOP::contains_box( const BoundingBox *box ) const
{
        nassertr( !is_empty(), 0 );
        nassertr( !box->is_empty(), 0 );

        // Put the box inside a sphere for the purpose of this test.
        const LPoint3 &min = box->get_minq();
        const LPoint3 &max = box->get_maxq();
        LPoint3 center = ( min + max ) * 0.5f;
        PN_stdfloat radius2 = ( max - center ).length_squared();

        int result = IF_possible | IF_some | IF_all;

        for ( size_t i = 0; i < _planes.size(); i++ )
        {
                const LPlane &p = _planes[i];
                PN_stdfloat dist = p.dist_to_plane( center );
                PN_stdfloat dist2 = dist * dist;

                if ( dist2 <= radius2 )
                {
                        // The sphere is not completely behind this plane, but some of it is.

                        // Look a little closer.
                        bool all_in = true;
                        bool all_out = true;
                        for ( int i = 0; i < 8 && ( all_in || all_out ); ++i )
                        {
                                if ( p.dist_to_plane( box->get_point( i ) ) < 0.0f )
                                {
                                        // This point is inside the plane.
                                        all_out = false;
                                }
                                else
                                {
                                   // This point is outside the plane.
                                        all_in = false;
                                }
                        }

                        if ( all_out )
                        {
                                return IF_no_intersection;
                        }
                        else if ( !all_in )
                        {
                                result &= ~IF_all;
                        }

                }
                else if ( dist >= 0.0f )
                {
                        // The sphere is completely in front of this plane.
                        return IF_no_intersection;
                }
        }

        return result;
}

int BoundingKDOP::contains_hexahedron( const BoundingHexahedron *hexahedron ) const
{
        nassertr( !is_empty(), 0 );
        nassertr( !hexahedron->is_empty(), 0 );

        // Put the hexahedron inside a sphere for the purposes of this test.
        LPoint3 min = hexahedron->get_min();
        LPoint3 max = hexahedron->get_max();
        LPoint3 center = ( min + max ) * 0.5f;
        PN_stdfloat radius2 = ( max - center ).length_squared();

        int result = IF_possible | IF_some | IF_all;

        for ( size_t i = 0; i < _planes.size(); i++ )
        {
                const LPlane &p = _planes[i];
                PN_stdfloat dist = p.dist_to_plane( center );
                PN_stdfloat dist2 = dist * dist;

                if ( dist >= 0.0f && dist2 > radius2 )
                {
                        // The sphere is completely in front of this plane; it's thus completely
                        // outside of the hexahedron.
                        return IF_no_intersection;

                }
                else /*if (dist < 0.0f && dist2 < radius2)*/
                {
                       // The sphere is not completely behind this plane, but some of it is.

                       // Look a little closer.
                        unsigned points_out = 0;
                        for ( int i = 0; i < 8; ++i )
                        {
                                if ( p.dist_to_plane( hexahedron->get_point( i ) ) > 0.0f )
                                {
                                        // This point is outside the plane.
                                        ++points_out;
                                }
                        }

                        if ( points_out != 0 )
                        {
                                if ( points_out == 8 )
                                {
                                        return IF_no_intersection;
                                }
                                result &= ~IF_all;
                        }
                }
        }

        return result;
}

void BoundingKDOP::set_points()
{
        
}

void BoundingKDOP::set_centroid()
{
        //LPoint3 net = _points[0];
        //for ( size_t i = 1; i < _points.size(); i++ )
        //{
        //        net += _points[i];
        //}
        //_centroid = net / (PN_stdfloat)_points.size();
}
