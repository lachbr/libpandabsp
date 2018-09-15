#ifndef BSP_TRACE_H
#define BSP_TRACE_H

#include <winding.h>
#include <bsptools.h>

/**
 * Enumerates the BSP tree checking for intersections with any non TEX_SPECIAL faces.
 */
class FaceFinder : public BaseBSPEnumerator
{
public:
        FaceFinder( bspdata_t *data );
        virtual bool enumerate_leaf( int node_id, const Ray &ray, float start, float end, int context );
        virtual bool find_intersection( const Ray &ray );
        virtual bool enumerate_node( int node_id, const Ray &ray, float f, int context );

};

#endif // BSP_TRACE_H