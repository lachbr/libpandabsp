#ifndef RAD_TRACE_H
#define RAD_TRACE_H

#include <simpleHashMap.h>

#include "raytrace.h"
#include "mathtypes.h"
#include "mathlib/ssemath.h"
#include "bspfile.h"

class RADTrace
{
public:
        static void initialize();

        static PT( RayTraceScene ) scene;

        static unsigned int test_line( const vec3_t start, const vec3_t stop, bool test_static_props = false );
        static void test_four_lines( const FourVectors &start, const FourVectors &end,
                                     fltx4 *fraction4, unsigned int contents_mask,
                                     bool test_static_props = false );
        static unsigned int test_line( const vec3_t start, const vec3_t end,
                                       float &fraction_visible, bool test_static_props = false );

        static BitMask32 world_mask;
        static BitMask32 props_mask;

        static dface_t *get_dface( const RayTraceHitResult &result );

        typedef SimpleHashMap<int, dface_t *, int_hash> PrimID2dface;
        typedef SimpleHashMap<int, PrimID2dface, int_hash> GeomID2PrimID2dface;
        static PrimID2dface dface_lookup;
};

#endif // RAD_TRACE_H