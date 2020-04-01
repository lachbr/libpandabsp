/**
*
* RenderPipeline
*
* Copyright (c) 2014-2016 tobspr <tobias.springer1@gmail.com>
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*
*/


#include "pssmCameraRig.h"

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <math.h>
#include "orthographicLens.h"
#include "boundingSphere.h"

#include "bounding_kdop.h"
#include "shader_generator.h"
#include <graphicsBuffer.h>
#include <lineSegs.h>
#include <collisionNode.h>
#include <collisionPlane.h>
#include <meshDrawer.h>

PStatCollector PSSMCameraRig::_update_collector( "App:CSM:Update" );
static PStatCollector create_union_collector( "App:CSM:Update:MakeCullBounds" );

static LightMutex csm_mutex( "CSMMutex" );

static MeshDrawer dbg_draw;
static NodePath dbg_root( "dbgRoot" );

#define CSM_BOUNDS_DEBUG 0
#define CSM_TIGHT_BOUNDS 0

/**
* @brief Constructs a new PSSM camera rig
* @details This constructs a new camera rig, with a given amount of splits.
*   The splits can not be changed later on. Splits are also called Cascades.
*
*   An assertion will be triggered if the splits are below zero.
*
* @param num_splits Amount of PSSM splits
*/
PSSMCameraRig::PSSMCameraRig( size_t num_splits, BSPShaderGenerator *gen )
{
	LightMutexHolder holder( csm_mutex );

        nassertv( num_splits > 0 );
        _num_splits = num_splits;
        _pssm_distance = 100.0;
        _sun_distance = 500.0;
        _use_fixed_film_size = false;
        _use_stable_csm = true;
        _logarithmic_factor = 1.0;
        _resolution = 512;
        _border_bias = 0.0;
        _camera_mvps = PTA_LMatrix4::empty_array( num_splits );
        _camera_nearfar = PTA_LVecBase2::empty_array( num_splits );
        _camera_viewmatrix = PTA_LMatrix4::empty_array( num_splits );
        _sun_vector = PTA_LVecBase3f::empty_array( 1 );
        _gen = gen;
        init_cam_nodes();
}

/**
* @brief Destructs the camera rig
* @details This destructs the camera rig, cleaning up all used resources.
*/
PSSMCameraRig::~PSSMCameraRig()
{
        // TODO: Detach all cameras and call remove_node. Most likely this is not
        // an issue tho, because the camera rig will never get destructed.
}

/**
* @brief Internal method to init the cameras
* @details This method constructs all cameras and their required lens nodes
*   for all splits. It also resets the film size array.
*/
void PSSMCameraRig::init_cam_nodes()
{
        _cam_nodes.reserve( _num_splits );
        _max_film_sizes.resize( _num_splits );
        _cameras.resize( _num_splits );
        for ( size_t i = 0; i < _num_splits; ++i )
        {
                // Construct a new lens
                Lens *lens = new OrthographicLens();
                lens->set_film_size( 1, 1 );
                lens->set_near_far( 1, 1000 );

                // Construct a new camera
                _cameras[i] = new Camera( "pssm-cam-" + format_string( i ), lens );
                _cameras[i]->set_camera_mask( BitMask32::bit( 5 ) );
                //_cameras[i]->show_frustum();
                _cam_nodes.push_back( NodePath( _cameras[i] ) );
                _max_film_sizes[i].fill( 0 );
        }
}

/**
* @brief Reparents the camera rig
* @details This reparents all cameras to the given parent. Usually the parent
*   will be ShowBase.render. The parent should be the same node where the
*   main camera is located in, too.
*
*   If an empty parrent is passed, an assertion will get triggered.
*
* @param parent Parent node path
*/
void PSSMCameraRig::reparent_to( NodePath parent )
{
        //nassertv( !parent.is_empty() );
        for ( size_t i = 0; i < _num_splits; ++i )
        {
                _cam_nodes[i].reparent_to( parent );
        }
        _parent = parent;
        dbg_draw.set_budget( 1000 );
        dbg_root = dbg_draw.get_root();
        dbg_root.reparent_to( _gen->_render );
        dbg_root.set_two_sided( true );
}

/**
* @brief Internal method to compute the view-projection matrix of a camera
* @details This returns the view-projection matrix of the given split. No bounds
*   checking is done. If an invalid index is passed, undefined behaviour occurs.
*
* @param split_index Index of the split
* @return view-projection matrix of the split
*/
LMatrix4 PSSMCameraRig::compute_mvp( size_t split_index )
{
        LMatrix4 transform = _parent.get_transform( _cam_nodes[split_index] )->get_mat();
        return transform * _cameras[split_index]->get_lens()->get_projection_mat();
}

/**
* @brief Internal method used for stable CSM
* @details This method is used when stable CSM is enabled. It ensures that each
*   source only moves in texel-steps, thus preventing flickering. This works by
*   projecting the point (0, 0, 0) to NDC space, making sure that it gets projected
*   to a texel center, and then projecting that texel back.
*
*   This only works if the camera does not rotate, change its film size, or change
*   its angle.
*
* @param mat view-projection matrix of the camera
* @param resolution resolution of the split
*
* @return Offset to add to the camera position to achieve stable snapping
*/
LVecBase3 PSSMCameraRig::get_snap_offset( const LMatrix4& mat, size_t resolution )
{
        // Transform origin to camera space
        LPoint4 base_point = mat.get_row( 3 ) * 0.5 + 0.5;

        // Compute the snap offset
        float texel_size = 1.0 / (float)( resolution );
        float offset_x = fmod( base_point.get_x(), texel_size );
        float offset_y = fmod( base_point.get_y(), texel_size );

        // Reproject the offset back, for that we need the inverse MVP
        LMatrix4 inv_mat( mat );
        inv_mat.invert_in_place();
        LVecBase3 new_base_point = inv_mat.xform_point( LVecBase3(
                ( base_point.get_x() - offset_x ) * 2.0 - 1.0,
                ( base_point.get_y() - offset_y ) * 2.0 - 1.0,
                base_point.get_z() * 2.0 - 1.0
        ) );
        return -new_base_point;
}

/**
* @brief Computes the average of a list of points
* @details This computes the average over a given set of points in 3D space.
*   It returns the average of those points, namely sum_of_points / num_points.
*
*   It is designed to work with a frustum, which is why it takes two arrays
*   with a dimension of 4. Usually the first array are the camera near points,
*   and the second array are the camera far points.
*
* @param starts First array of points
* @param ends Second array of points
* @return Average of points
*/
LPoint3 get_average_of_points( LVecBase3 const ( &starts )[4], LVecBase3 const ( &ends )[4] )
{
        LPoint3 mid_point( 0, 0, 0 );
        for ( size_t k = 0; k < 4; ++k )
        {
                mid_point += starts[k];
                mid_point += ends[k];
        }
        return mid_point / 8.0;
}

/**
* @brief Finds the minimum and maximum extends of the given projection
* @details This projects each point of the given array of points using the
*   cameras view-projection matrix, and computes the minimum and maximum
*   of the projected points.
*
* @param min_extent Will store the minimum extent of the projected points in NDC space
* @param max_extent Will store the maximum extent of the projected points in NDC space
* @param transform The transformation matrix of the camera
* @param proj_points The array of points to project
* @param cam The camera to be used to project the points
*/
void find_min_max_extents( LVecBase3 &min_extent, LVecBase3 &max_extent, const LMatrix4 &transform, LVecBase3 const ( &proj_points )[8], Camera *cam )
{

        min_extent.fill( 1e10 );
        max_extent.fill( -1e10 );
        LPoint2 screen_points[8];

        // Now project all points to the screen space of the current camera and also
        // find the minimum and maximum extents
        for ( size_t k = 0; k < 8; ++k )
        {
                LVecBase4 point( proj_points[k], 1 );
                LPoint4 proj_point = transform.xform( point );
                LPoint3 proj_point_3d( proj_point.get_x(), proj_point.get_y(), proj_point.get_z() );
                cam->get_lens()->project( proj_point_3d, screen_points[k] );

                // Find min / max extents
                if ( screen_points[k].get_x() > max_extent.get_x() ) max_extent.set_x( screen_points[k].get_x() );
                if ( screen_points[k].get_y() > max_extent.get_y() ) max_extent.set_y( screen_points[k].get_y() );

                if ( screen_points[k].get_x() < min_extent.get_x() ) min_extent.set_x( screen_points[k].get_x() );
                if ( screen_points[k].get_y() < min_extent.get_y() ) min_extent.set_y( screen_points[k].get_y() );

                // Find min / max projected depth to adjust far plane
                if ( proj_point.get_y() > max_extent.get_z() ) max_extent.set_z( proj_point.get_y() );
                if ( proj_point.get_y() < min_extent.get_z() ) min_extent.set_z( proj_point.get_y() );
        }
}

/**
* @brief Computes a film size from a given minimum and maximum extend
* @details This takes a minimum and maximum extent in NDC space and computes
*   the film size and film offset needed to cover that extent.
*
* @param film_size Output film size, can be used for Lens::set_film_size
* @param film_offset Output film offset, can be used for Lens::set_film_offset
* @param min_extent Minimum extent
* @param max_extent Maximum extent
*/
inline void get_film_properties( LVecBase2 &film_size, LVecBase2 &film_offset, const LVecBase3 &min_extent, const LVecBase3 &max_extent )
{
        float x_center = ( min_extent.get_x() + max_extent.get_x() ) * 0.5;
        float y_center = ( min_extent.get_y() + max_extent.get_y() ) * 0.5;
        float x_size = max_extent.get_x() - x_center;
        float y_size = max_extent.get_y() - y_center;
        film_size.set( x_size, y_size );
        film_offset.set( x_center * 0.5, y_center * 0.5 );
}

/**
* @brief Merges two arrays
* @details This takes two arrays which each 4 members and produces an array
*   with both arrays contained.
*
* @param dest Destination array
* @param array1 First array
* @param array2 Second array
*/
inline void merge_points_interleaved( LVecBase3( &dest )[8], LVecBase3 const ( &array1 )[4], LVecBase3 const ( &array2 )[4] )
{
        for ( size_t k = 0; k < 4; ++k )
        {
                dest[k] = array1[k];
                dest[k + 4] = array2[k];
        }
}

class FrustumHelper
{
public:
        /** Frustum planes. */
        enum
        {
                PLANE_NEAR,
                PLANE_TOP,
                PLANE_LEFT,
                PLANE_BOTTOM,
                PLANE_RIGHT,
                PLANE_FAR,
                PLANE_TOTAL
        };

        /** Frustum points. */
        enum
        {
                POINT_FAR_BOTTOM_LEFT,
                POINT_FAR_BOTTOM_RIGHT,
                POINT_FAR_TOP_RIGHT,
                POINT_FAR_TOP_LEFT,       
                
                POINT_NEAR_BOTTOM_LEFT,
                POINT_NEAR_BOTTOM_RIGHT,
                POINT_NEAR_TOP_RIGHT,
                POINT_NEAR_TOP_LEFT,
        };

        INLINE static void get_corners_of_planes( int plane1, int plane2, int points[2] )
        {
                static const int table[PLANE_TOTAL][PLANE_TOTAL][2] = {
                        // ==
                        {	// PLANE_NEAR
                                {	// PLANE_NEAR
                                        POINT_NEAR_TOP_LEFT, POINT_NEAR_TOP_RIGHT,		// Invalid combination.
                                },
                                {	// PLANE_TOP
                                        POINT_NEAR_TOP_RIGHT, POINT_NEAR_TOP_LEFT,
                                },
                                {	// PLANE_LEFT
                                        POINT_NEAR_TOP_LEFT, POINT_NEAR_BOTTOM_LEFT,
                                },
                                {	// PLANE_BOTTOM
                                        POINT_NEAR_BOTTOM_LEFT, POINT_NEAR_BOTTOM_RIGHT,
                                },
                                {	// PLANE_RIGHT
                                        POINT_NEAR_BOTTOM_RIGHT, POINT_NEAR_TOP_RIGHT,
                                },
                                {	// PLANE_FAR
                                        POINT_FAR_TOP_RIGHT, POINT_FAR_TOP_LEFT,		// Invalid combination.
                                },

                        },
                        // ==
                        {	// PLANE_TOP
                                {	// PLANE_NEAR
                                        POINT_NEAR_TOP_LEFT, POINT_NEAR_TOP_RIGHT,
                                },
                                {	// PLANE_TOP
                                        POINT_NEAR_TOP_LEFT, POINT_FAR_TOP_LEFT,		// Invalid combination.
                                },
                                {	// PLANE_LEFT
                                        POINT_FAR_TOP_LEFT, POINT_NEAR_TOP_LEFT,
                                },
                                {	// PLANE_BOTTOM
                                        POINT_FAR_BOTTOM_LEFT, POINT_NEAR_BOTTOM_LEFT,	// Invalid combination.
                                },
                                {	// PLANE_RIGHT
                                        POINT_NEAR_TOP_RIGHT, POINT_FAR_TOP_RIGHT,
                                },
                                {	// PLANE_FAR
                                        POINT_FAR_TOP_RIGHT, POINT_FAR_TOP_LEFT,
                                },
                        },
                        {	// PLANE_LEFT
                                {	// PLANE_NEAR
                                        POINT_NEAR_BOTTOM_LEFT, POINT_NEAR_TOP_LEFT,
                                },
                                {	// PLANE_TOP
                                        POINT_NEAR_TOP_LEFT, POINT_FAR_TOP_LEFT,
                                },
                                {	// PLANE_LEFT
                                        POINT_FAR_BOTTOM_LEFT, POINT_FAR_BOTTOM_LEFT,		// Invalid combination.
                                },
                                {	// PLANE_BOTTOM
                                        POINT_FAR_BOTTOM_LEFT, POINT_NEAR_BOTTOM_LEFT,
                                },
                                {	// PLANE_RIGHT
                                        POINT_FAR_BOTTOM_LEFT, POINT_FAR_BOTTOM_LEFT,		// Invalid combination.
                                },
                                {	// PLANE_FAR
                                        POINT_FAR_TOP_LEFT, POINT_FAR_BOTTOM_LEFT,
                                },
                        },
                        {	// PLANE_BOTTOM
                                {	// PLANE_NEAR
                                        POINT_NEAR_BOTTOM_RIGHT, POINT_NEAR_BOTTOM_LEFT,
                                },
                                {	// PLANE_TOP
                                        POINT_NEAR_BOTTOM_LEFT, POINT_FAR_BOTTOM_LEFT,	// Invalid combination.
                                },
                                {	// PLANE_LEFT
                                        POINT_NEAR_BOTTOM_LEFT, POINT_FAR_BOTTOM_LEFT,
                                },
                                {	// PLANE_BOTTOM
                                        POINT_FAR_BOTTOM_LEFT, POINT_NEAR_BOTTOM_LEFT,	// Invalid combination.
                                },
                                {	// PLANE_RIGHT
                                        POINT_FAR_BOTTOM_RIGHT, POINT_NEAR_BOTTOM_RIGHT,
                                },
                                {	// PLANE_FAR
                                        POINT_FAR_BOTTOM_LEFT, POINT_FAR_BOTTOM_RIGHT,
                                },
                        },
                        {	// PLANE_RIGHT
                                {	// PLANE_NEAR
                                        POINT_NEAR_TOP_RIGHT, POINT_NEAR_BOTTOM_RIGHT,
                                },
                                {	// PLANE_TOP
                                        POINT_FAR_TOP_RIGHT, POINT_NEAR_TOP_RIGHT,
                                },
                                {	// PLANE_LEFT
                                        POINT_FAR_BOTTOM_RIGHT, POINT_FAR_BOTTOM_RIGHT,	// Invalid combination.
                                },
                                {	// PLANE_BOTTOM
                                        POINT_NEAR_BOTTOM_RIGHT, POINT_FAR_BOTTOM_RIGHT,
                                },                                
                                {	// PLANE_RIGHT
                                        POINT_FAR_BOTTOM_RIGHT, POINT_FAR_BOTTOM_RIGHT,	// Invalid combination.
                                },
                                {	// PLANE_FAR
                                        POINT_FAR_BOTTOM_RIGHT, POINT_FAR_TOP_RIGHT,
                                },
                        },
                        {	// PLANE_FAR
                                {	// PLANE_NEAR
                                        POINT_FAR_TOP_LEFT, POINT_FAR_TOP_RIGHT,		// Invalid combination.
                                },
                                {	// PLANE_TOP
                                        POINT_FAR_TOP_LEFT, POINT_FAR_TOP_RIGHT,
                                },
                                {	// PLANE_LEFT
                                        POINT_FAR_BOTTOM_LEFT, POINT_FAR_TOP_LEFT,
                                },
                                {	// PLANE_BOTTOM
                                        POINT_FAR_BOTTOM_RIGHT, POINT_FAR_BOTTOM_LEFT,
                                },
                                {	// PLANE_RIGHT
                                        POINT_FAR_TOP_RIGHT, POINT_FAR_BOTTOM_RIGHT,
                                },
                                {	// PLANE_FAR
                                        POINT_FAR_TOP_RIGHT, POINT_FAR_TOP_LEFT,		// Invalid combination.
                                },
                        },
                        
                };
                points[0] = table[plane1][plane2][0];
                points[1] = table[plane1][plane2][1];
        }

        INLINE static std::string get_plane_name( int planenum )
        {
                switch ( planenum )
                {
                case PLANE_NEAR:
                        return "near";
                case PLANE_FAR:
                        return "far";
                case PLANE_LEFT:
                        return "left";
                case PLANE_RIGHT:
                        return "right";
                case PLANE_BOTTOM:
                        return "bottom";
                case PLANE_TOP:
                        return "top";
                default:
                        return "unknown";
                }
        }

        INLINE static std::string get_point_name( int pointnum )
        {
                switch ( pointnum )
                {
                case POINT_FAR_BOTTOM_LEFT:
                        return "fll";
                case POINT_FAR_BOTTOM_RIGHT:
                        return "flr";
                case POINT_FAR_TOP_LEFT:
                        return "ful";
                case POINT_FAR_TOP_RIGHT:
                        return "fur";

                case POINT_NEAR_BOTTOM_LEFT:
                        return "nll";
                case POINT_NEAR_BOTTOM_RIGHT:
                        return "nlr";
                case POINT_NEAR_TOP_LEFT:
                        return "nul";
                case POINT_NEAR_TOP_RIGHT:
                        return "nur";

                default:
                        return "unknown";
                }
        }

        INLINE static void get_neighbors( int planenum, int neighbors[4] )
        {
                static const int planes[PLANE_TOTAL][4] = {
                        {	// PLANE_NEAR
                                PLANE_LEFT,
                                PLANE_RIGHT,
                                PLANE_TOP,
                                PLANE_BOTTOM
                        },
                        {	// PLANE_TOP
                                PLANE_LEFT,
                                PLANE_RIGHT,
                                PLANE_NEAR,
                                PLANE_FAR
                        },
                        {	// PLANE_LEFT
                                PLANE_TOP,
                                PLANE_BOTTOM,
                                PLANE_NEAR,
                                PLANE_FAR
                        },
                        {	// PLANE_BOTTOM
                                PLANE_LEFT,
                                PLANE_RIGHT,
                                PLANE_NEAR,
                                PLANE_FAR
                        },
                        {	// PLANE_RIGHT
                                PLANE_TOP,
                                PLANE_BOTTOM,
                                PLANE_NEAR,
                                PLANE_FAR
                        },                        
                        {	// PLANE_FAR
                                PLANE_LEFT,
                                PLANE_RIGHT,
                                PLANE_TOP,
                                PLANE_BOTTOM
                        },
                        
                        
                };

                for ( int i = 0; i < 4; i++ )
                {
                        neighbors[i] = planes[planenum][i];
                }
        }
};

static bool three_planes( const LPlane &p0, const LPlane &p1, const LPlane &p2, LPoint3 &pt )
{
        LVector3 u = p1.get_normal().cross( p2.get_normal() );
        float denom = p0.get_normal().dot( u );
        if ( fabsf( denom ) <= FLT_EPSILON )
                return false;
        pt = ( u * p0[3] + p0.get_normal().cross( p1.get_normal() * p2[3] - p2.get_normal() * p1[3] ) ) / denom;
        return true;
}

static PT( BoundingKDOP ) make_shadow_cull_bounds( const BoundingHexahedron *view_frustum, const LVector3 &light_dir, int &backplanes )
{
        pvector<LPlane> planes;

        // Add the planes that are facing towards us.
        for ( int i = 0; i < 6; i++ )
        {
                const LPlane &plane = view_frustum->get_plane( i );
                float dot = plane.get_normal().dot( light_dir );
                if ( dot < 0 )
                {
                        planes.push_back( plane );
                }
                        
        }
        backplanes = (int)planes.size();
#if CSM_BOUNDS_DEBUG
        std::cout << "Points of frustum:" << std::endl;
        for ( size_t i = 0; i < view_frustum->get_num_points(); i++ )
        {
                std::cout << "\t" << FrustumHelper::get_point_name( i ) << ": " << view_frustum->get_point( i ) << std::endl;
        }
#endif

        // We have added the back sides of the planes.
        // Now find the edges for each plane.
        for ( int i = 0; i < 6; i++ )
        {
                const LPlane &plane = view_frustum->get_plane( i );
                float dot = plane.get_normal().dot( light_dir );
                if ( dot > 0 )
                        continue;

                // For each neighbor of this plane.
                int neighbors[4];
                FrustumHelper::get_neighbors( i, neighbors );

                for ( int j = 0; j < 4; j++ )
                {
                        const LPlane &neighbor_plane = view_frustum->get_plane( neighbors[j] );
                        if ( neighbor_plane.get_normal().dot( light_dir ) > 0 )
                        {
                                int points[2];
                                FrustumHelper::get_corners_of_planes( i, neighbors[j], points );
#if CSM_BOUNDS_DEBUG
                                std::cout << "Corners of planes " << FrustumHelper::get_plane_name( i ) << " and "
                                        << FrustumHelper::get_plane_name( neighbors[j] ) << ":" << std::endl;
                                for ( size_t x = 0; x < 2; x++ )
                                {
                                        std::cout << "\t" << FrustumHelper::get_point_name( points[x] ) << std::endl;
                                }
#endif
                                planes.push_back( LPlane( view_frustum->get_point( points[0] ),
                                                          view_frustum->get_point( points[1] ),
                                                          view_frustum->get_point( points[0] ) + light_dir ) );
                                        
                        }
                }
        }

        return new BoundingKDOP( planes );
}


/**
* @brief Internal method to compute the splits
* @details This is the internal update method to update the PSSM splits.
*   It distributes the camera splits over the frustum, and updates the
*   MVP array aswell as the nearfar array.
*
* @param transform Main camera transform
* @param max_distance Maximum pssm distance, relative to the camera far plane
* @param light_vector Sun-Vector
*/
void PSSMCameraRig::compute_pssm_splits( const LMatrix4& transform, float max_distance,
                                         const LVecBase3& light_vector, Camera *main_cam )
{
        nassertv( !_parent.is_empty() );

        // PSSM Distance should never be smaller than camera far plane.
        nassertv( max_distance <= 1.0 );

        float filmsize_bias = 1.0 + _border_bias;

        // Compute the positions of all cameras
        for ( size_t i = 0; i < _cam_nodes.size(); ++i )
        {
                float split_start = get_split_start( i ) * max_distance;
                float split_end = get_split_start( i + 1 ) * max_distance;

                LVecBase3 start_points[4];
                LVecBase3 end_points[4];
                LVecBase3 proj_points[8];

                // Get split bounding box, and collect all points which define the frustum
                for ( size_t k = 0; k < 4; ++k )
                {
                        start_points[k] = get_interpolated_point( (CoordinateOrigin)k, split_start );
                        end_points[k] = get_interpolated_point( (CoordinateOrigin)k, split_end );

                        proj_points[k] = start_points[k];
                        proj_points[k + 4] = end_points[k];
                }

                // Compute approximate split mid point
                LPoint3 split_mid = get_average_of_points( start_points, end_points );
                LPoint3 cam_start = split_mid + light_vector * _sun_distance;

                // Reset the film size, offset and far-plane
                Camera* cam = DCAST( Camera, _cam_nodes[i].node() );
                cam->get_lens()->set_film_size( 1, 1 );
                cam->get_lens()->set_film_offset( 0, 0 );
                cam->get_lens()->set_near_far( 1, 100 );

                // Find a good initial position
                _cam_nodes[i].set_pos( cam_start );
                _cam_nodes[i].look_at( split_mid );

                LVecBase3 best_min_extent, best_max_extent;

                // Find minimum and maximum extents of the points
                LMatrix4 merged_transform = _parent.get_transform( _cam_nodes[i] )->get_mat();
                find_min_max_extents( best_min_extent, best_max_extent, merged_transform, proj_points, cam );

                // Find the film size to cover all points
                LVecBase2 film_size, film_offset;
                get_film_properties( film_size, film_offset, best_min_extent, best_max_extent );

                if ( _use_fixed_film_size )
                {
                        // In case we use a fixed film size, store the maximum film size, and
                        // only change the film size if a new maximum is there
                        if ( _max_film_sizes[i].get_x() < film_size.get_x() ) _max_film_sizes[i].set_x( film_size.get_x() );
                        if ( _max_film_sizes[i].get_y() < film_size.get_y() ) _max_film_sizes[i].set_y( film_size.get_y() );

                        cam->get_lens()->set_film_size( _max_film_sizes[i] * filmsize_bias );
                } else
                {
                        // If we don't use a fixed film size, we can just set the film size
                        // on the lens.
                        cam->get_lens()->set_film_size( film_size * filmsize_bias );
                }

                // Compute new film offset
                cam->get_lens()->set_film_offset( film_offset );
                
                _camera_nearfar[i] = LVecBase2( best_min_extent.get_z(), best_max_extent.get_z() );
                cam->get_lens()->set_near_far( 10.0, _sun_distance * 2 );

                // Compute the camera MVP
                LMatrix4 mvp = compute_mvp( i );

                // Stable CSM Snapping
                if ( _use_stable_csm )
                {
                        LPoint3 snap_offset = get_snap_offset( mvp, _resolution );
                        _cam_nodes[i].set_pos( _cam_nodes[i].get_pos() + snap_offset );

                        // Compute the new mvp, since we changed the snap offset
                        mvp = compute_mvp( i );
                }

                _camera_viewmatrix.set_element( i, merged_transform );
                _camera_mvps.set_element( i, mvp );
        }

#if CSM_TIGHT_BOUNDS
        create_union_collector.start();
        
        // Generate a tight shadow camera frustum to only
        // render objects into the shadow maps that may
        // possibly cast a shadow into the view frustum.
        PT( BoundingHexahedron ) world_view_frustum = DCAST( BoundingHexahedron, main_cam->get_lens()->make_bounds() );
        //LMatrix4 cam_mat = NodePath( main_cam ).get_net_transform()->get_mat();
        //world_view_frustum->xform( cam_mat );

        int backplanes;
        PT( BoundingKDOP ) bounds = make_shadow_cull_bounds(
                world_view_frustum,
                NodePath( main_cam ).get_net_transform()->get_inverse()->get_mat().xform_vec( -light_vector ),
                backplanes );

#if CSM_BOUNDS_DEBUG
        PT( BoundingKDOP ) dbg_bounds = DCAST( BoundingKDOP, bounds->make_copy() );
        // Print planes in camera space for the sake of simplicity.
        //dbg_bounds->xform( NodePath( main_cam ).get_net_transform()->get_inverse()->get_mat() );
        std::cout << dbg_bounds->get_num_planes() << " planes" << std::endl;
        for ( size_t i = 0; i < dbg_bounds->get_num_planes(); i++ )
        {
                std::cout << "\t" << dbg_bounds->get_plane( i ) << std::endl;
        }
        dbg_draw.begin( _gen->_camera, _gen->_render );
        // generate geometry for the planes
        for ( size_t i = 0; i < backplanes; i++ )
        {
                for ( size_t j = 0; j < backplanes; j++ )
                {
                        for ( size_t k = 0; k < backplanes; k++ )
                        {
                                LPoint3 vertex;
                                if ( three_planes( bounds->get_plane( i ),
                                                   bounds->get_plane( j ),
                                                   bounds->get_plane( k ),
                                                   vertex ) )
                                {
                                        // put a point at this vertex
                                        dbg_draw.billboard( vertex, LVector4( -1, 1, -1, 1 ), 0.3, LVector4( 1, 0, 0, 1 ) );
;                                }
                        }
                }
        }
        dbg_draw.end();
        dbg_root.reparent_to( NodePath(main_cam) );
#endif // CSM_BOUNDS_DEBUG
        
        Camera *maincam = DCAST( Camera, _cam_nodes[0].node() );
        maincam->set_cull_center( NodePath(main_cam) );
        maincam->set_cull_bounds( bounds );
        maincam->set_final( true );


        create_union_collector.stop();

#endif // CSM_TIGHT_BOUNDS
}


/**
* @brief Updates the PSSM camera rig
* @details This updates the rig with an updated camera position, and a given
*   light vector. This should be called on a per-frame basis. It will reposition
*   all camera sources to fit the frustum based on the pssm distribution.
*
*   The light vector should be the vector from the light source, not the
*   vector to the light source.
*
* @param cam_node Target camera node
* @param light_vector The vector from the light to any point
*/
void PSSMCameraRig::update( NodePath cam_node, const LVecBase3 &light_vector )
{
	LightMutexHolder holder( csm_mutex );

	if ( !_is_setup )
		return;

        nassertv( !cam_node.is_empty() );
        _update_collector.start();

        _sun_vector.set_element( 0, light_vector );

        // Get camera node transform
        LMatrix4 transform = cam_node.get_net_transform()->get_mat();

        // Get Camera and Lens pointers
	Camera* cam = DCAST( Camera, cam_node.node() );//cam_node.get_child( 0 ).node() );
        nassertv( cam != nullptr );
        Lens* lens = cam->get_lens();

        // Extract near and far points:
        lens->extrude( LPoint2( -1, 1 ), _curr_near_points[UpperLeft], _curr_far_points[UpperLeft] );
        lens->extrude( LPoint2( 1, 1 ), _curr_near_points[UpperRight], _curr_far_points[UpperRight] );
        lens->extrude( LPoint2( -1, -1 ), _curr_near_points[LowerLeft], _curr_far_points[LowerLeft] );
        lens->extrude( LPoint2( 1, -1 ), _curr_near_points[LowerRight], _curr_far_points[LowerRight] );

        // Construct MVP to project points to world space
        LMatrix4 mvp = transform * lens->get_view_mat();

        // Project all points to world space
        for ( size_t i = 0; i < 4; ++i )
        {
                LPoint4 ws_near = mvp.xform( _curr_near_points[i] );
                LPoint4 ws_far = mvp.xform( _curr_far_points[i] );
                _curr_near_points[i].set( ws_near.get_x(), ws_near.get_y(), ws_near.get_z() );
                _curr_far_points[i].set( ws_far.get_x(), ws_far.get_y(), ws_far.get_z() );
        }

        // Do the actual PSSM
        compute_pssm_splits( transform, _pssm_distance / lens->get_far(), light_vector, cam );

        _update_collector.stop();
}

/**
* @brief Sets the maximum pssm distance.
* @details This sets the maximum distance in world space until which shadows
*   are rendered. After this distance, no shadows will be rendered.
*
*   If the distance is below zero, an assertion is triggered.
*
* @param distance Maximum distance in world space
*/
void PSSMCameraRig::set_pssm_distance( float distance )
{
        nassertv( distance > 0.0 && distance < 100000.0 );
        _pssm_distance = distance;
}

/**
* @brief Sets the suns distance
* @details This sets the distance the cameras will have from the cameras frustum.
*   This prevents far objects from having no shadows, which can occur when these
*   objects are between the cameras frustum and the sun, but not inside of the
*   cameras frustum. Setting the sun distance high enough will move the cameras
*   away from the camera frustum, being able to cover those distant objects too.
*
*   If the sun distance is set too high, artifacts will occur due to the reduced
*   range of depth. If a value below zero is passed, an assertion will get
*   triggered.
*
* @param distance The sun distance
*/
void PSSMCameraRig::set_sun_distance( float distance )
{
        nassertv( distance > 0.0 && distance < 100000.0 );
        _sun_distance = distance;
}

/**
* @brief Sets the logarithmic factor
* @details This sets the logarithmic factor, which is the core of the algorithm.
*   PSSM splits the camera frustum based on a linear and a logarithmic factor.
*   While a linear factor provides a good distribution, it often is not applicable
*   for wider distances. A logarithmic distribution provides a better distribution
*   at distance, but suffers from splitting in the near areas.
*
*   The logarithmic factor mixes the logarithmic and linear split distribution,
*   to get the best of both. A greater factor will make the distribution more
*   logarithmic, while a smaller factor will make it more linear.
*
*   If the factor is below zero, an ssertion is triggered.
*
* @param factor The logarithmic factor
*/
void PSSMCameraRig::set_logarithmic_factor( float factor )
{
        nassertv( factor > 0.0 );
        _logarithmic_factor = factor;
}

/**
* @brief Sets whether to use a fixed film size
* @details This controls if a fixed film size should be used. This will cause
*   the camera rig to cache the current film size, and only change it in case
*   it gets too small. This provides less flickering when moving, because the
*   film size will stay roughly constant. However, to prevent the cached film
*   size getting too big, one should call PSSMCameraRig::reset_film_size
*   once in a while, otherwise there might be a lot of wasted space.
*
* @param flag Whether to use a fixed film size
*/
void PSSMCameraRig::set_use_fixed_film_size( bool flag )
{
        _use_fixed_film_size = flag;
}

/**
* @brief Sets the resolution of each split
* @details This sets the resolution of each split. Currently it is equal for
*   each split. This is required when using PSSMCameraRig::set_use_stable_csm,
*   to compute how bix a texel is.
*
*   It has to match the y-resolution of the pssm shadow map. If an invalid
*   resolution is triggered, an assertion is thrown.
*
* @param resolution The resolution of each split.
*/
void PSSMCameraRig::set_resolution( size_t resolution )
{
        nassertv( resolution >= 0 && resolution < 65535 );
        _resolution = resolution;
}

/**
* @brief Sets whether to use stable CSM snapping.
* @details This option controls if stable CSM snapping should be used. When the
*   option is enabled, all splits will snap to their texels, so that when moving,
*   no flickering will occur. However, this only works when the splits do not
*   change their film size, rotation and angle.
*
* @param flag Whether to use stable CSM snapping
*/
void PSSMCameraRig::set_use_stable_csm( bool flag )
{
        _use_stable_csm = flag;
}

/**
* @brief Sets the border bias for each split
* @details This sets the border bias for every split. This increases each
*   splits frustum by multiplying it by (1 + bias), and helps reducing artifacts
*   at the borders of the splits. Artifacts can occur when the bias is too low,
*   because then the filtering will go over the bounds of the split, producing
*   invalid results.
*
*   If the bias is below zero, an assertion is thrown.
*
* @param bias Border bias
*/
void PSSMCameraRig::set_border_bias( float bias )
{
        nassertv( bias >= 0.0 );
        _border_bias = bias;
}

/**
* @brief Resets the film size cache
* @details In case PSSMCameraRig::set_use_fixed_film_size is used, this resets
*   the film size cache. This might lead to a small "jump" in the shadows,
*   because the film size changes, however it leads to a better shadow distribution.
*
*   This is the case because when using a fixed film size, the cache will get
*   bigger and bigger, whenever the camera moves to a grazing angle. However,
*   when moving back to a normal angle, the film size cache still stores this
*   big angle, and thus the splits will have a much bigger film size than actualy
*   required. To prevent this, call this method once in a while, so an optimal
*   distribution is ensured.
*/
void PSSMCameraRig::reset_film_size_cache()
{
        for ( size_t i = 0; i < _max_film_sizes.size(); ++i )
        {
                _max_film_sizes[i].fill( 0 );
        }
}

/**
* @brief Returns the n-th camera
* @details This returns the n-th camera of the camera rig, which can be used
*   for various stuff like showing its frustum, passing it as a shader input,
*   and so on.
*
*   The first camera is the camera which is the camera of the first split,
*   which is the split closest to the camera. All cameras follow in descending
*   order until to the last camera, which is the split furthest away from the
*   camera.
*
*   If an invalid index is passed, an assertion is thrown.
*
* @param index Index of the camera.
* @return [description]
*/
NodePath PSSMCameraRig::get_camera( size_t index )
{
        nassertr( index >= 0 && index < _cam_nodes.size(), NodePath() );
        return _cam_nodes[index];
}

/**
* @brief Internal method to compute the distance of a split
* @details This is the internal method to perform the weighting of the
*   logarithmic and linear distribution. It computes the distance to the
*   camera from which a given split starts, by weighting the logarithmic and
*   linear factor.
*
*   The return value is a value ranging from 0 .. 1. To get the distance in
*   world space, the value has to get multiplied with the maximum shadow distance.
*
* @param split_index The index of the split
* @return Distance of the split, ranging from 0 .. 1
*/
inline float PSSMCameraRig::get_split_start( size_t split_index )
{
        float x = (float)split_index / (float)_cam_nodes.size();
        return ( ( exp( _logarithmic_factor*x ) - 1 ) / ( exp( _logarithmic_factor ) - 1 ) );
}

/**
* @brief Internal method for interpolating a point along the camera frustum
* @details This method takes a given distance in the 0 .. 1 range, whereas
*   0 denotes the camera near plane, and 1 denotes the camera far plane,
*   and lineary interpolates between them.
*
* @param origin Edge of the frustum
* @param depth Distance in the 0 .. 1 range
*
* @return interpolated point in world space
*/
inline LPoint3 PSSMCameraRig::get_interpolated_point( CoordinateOrigin origin, float depth )
{
        nassertr( depth >= 0.0 && depth <= 1.0, LPoint3() );
        return _curr_near_points[origin] * ( 1.0 - depth ) + _curr_far_points[origin] * depth;
}

/**
* @brief Returns a handle to the MVP array
* @details This returns a handle to the array of view-projection matrices
*   of the different splits. This can be used for computing shadows. The array
*   is a PTALMatrix4 and thus can be directly bound to a shader.
*
* @return view-projection matrix array
*/
const PTA_LMatrix4 &PSSMCameraRig::get_mvp_array()
{
        return _camera_mvps;
}

const PTA_LMatrix4 &PSSMCameraRig::get_viewmatrix_array()
{
        return _camera_viewmatrix;
}

const PTA_LVecBase3f &PSSMCameraRig::get_sun_vector()
{
        return _sun_vector;
}

/**
* @brief Returns a handle to the near and far planes array
* @details This returns a handle to the near and far plane array. Each split
*   has an entry in the array, whereas the x component of the vecto denotes the
*   near plane, and the y component denotes the far plane of the split.
*
*   This is required because the near and far planes of the splits change
*   constantly. To access them in a shader, the shader needs access to the
*   array.
*
* @return Array of near and far planes
*/
const PTA_LVecBase2 &PSSMCameraRig::get_nearfar_array()
{
        return _camera_nearfar;
}
