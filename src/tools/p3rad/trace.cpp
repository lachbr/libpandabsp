#include "trace.h"

#include "cmdlib.h"
#include "mathlib.h"
#include "bspfile.h"
#include "log.h" //--vluzacn
#include "winding.h"
//#ifdef HLRAD_OPAQUE_ALPHATEST
#include "qrad.h"
//#endif
#include "radstaticprop.h"

#include <pStatCollector.h>
#include <pStatTimer.h>

static PStatCollector testline_collector( "RadWorld:TestLine" );
static PStatCollector test4lines_collector( "RadWorld:TestFourLines" );

static const unsigned int ALL_CONTENTS = (
        CONTENTS_EMPTY |
        CONTENTS_SOLID |
        CONTENTS_WATER |
        CONTENTS_SLIME |
        CONTENTS_LAVA |
        CONTENTS_SKY |
        CONTENTS_ORIGIN |

        CONTENTS_CURRENT_0 |
        CONTENTS_CURRENT_90 |
        CONTENTS_CURRENT_180 |
        CONTENTS_CURRENT_270 |
        CONTENTS_CURRENT_UP |
        CONTENTS_CURRENT_DOWN |

        CONTENTS_TRANSLUCENT |
        CONTENTS_HINT |

        CONTENTS_NULL |


        CONTENTS_BOUNDINGBOX |

        CONTENTS_TOEMPTY
        );

BitMask32 RADTrace::world_mask = BitMask32::bit( 1 );
BitMask32 RADTrace::props_mask = BitMask32::bit( 2 );
PT( RayTraceScene ) RADTrace::scene = nullptr;
RADTrace::PrimID2dface RADTrace::dface_lookup;

static const unsigned int ALL_CONTENTS_OR_PROPS = ALL_CONTENTS | CONTENTS_PROP;
static const u32x4 Four_ALL_CONTENTS = { ALL_CONTENTS, ALL_CONTENTS, ALL_CONTENTS, ALL_CONTENTS };
static const u32x4 Four_ALL_CONTENTS_OR_PROPS = { ALL_CONTENTS_OR_PROPS, ALL_CONTENTS_OR_PROPS, ALL_CONTENTS_OR_PROPS, ALL_CONTENTS_OR_PROPS };

void RADTrace::initialize()
{
        static bool initialized = false;
        if ( initialized )
                return;

        initialized = true;

        scene = new RayTraceScene;
}

unsigned int RADTrace::test_line( const vec3_t start, const vec3_t stop,
                                  bool test_static_props )
{
        //PStatTimer timer( testline_collector );

        BitMask32 mask = test_static_props ? ALL_CONTENTS | CONTENTS_PROP : ALL_CONTENTS;

        RayTraceHitResult result = scene->trace_line( LPoint3( start[0], start[1], start[2] ),
                                                      LPoint3( stop[0], stop[1], stop[2] ),
                                                      mask );
        if ( !result.hit )
        {
                return CONTENTS_EMPTY;
        }
        // Return the contents of what we hit
        return scene->get_geometry( result.geom_id )->get_mask().get_word();
}

unsigned int RADTrace::test_line( const vec3_t start, const vec3_t stop, float &fraction_visible,
                                  bool test_static_props )
{
        //PStatTimer timer( testline_collector );

        BitMask32 mask = test_static_props ? ALL_CONTENTS | CONTENTS_PROP : ALL_CONTENTS;
        RayTraceHitResult result = scene->trace_line( LPoint3( start[0], start[1], start[2] ),
                                                      LPoint3( stop[0], stop[1], stop[2] ),
                                                      mask );
        fraction_visible = result.hit_fraction;
        if ( !result.hit )
        {
                return CONTENTS_EMPTY;
        }
        // Return the contents of what we hit
        return scene->get_geometry( result.geom_id )->get_mask().get_word();
}

void RADTrace::test_four_lines( const FourVectors &start, const FourVectors &end, fltx4 *fraction4,
                                unsigned int contents_mask,
                                bool test_static_props )
{
        //PStatTimer timer( test4lines_collector );

        RayTraceHitResult4 result;

        float frac_vis;
        int contents;

        scene->trace_four_lines( start, end, test_static_props ? Four_ALL_CONTENTS_OR_PROPS : Four_ALL_CONTENTS, &result );

        for ( int i = 0; i < 4; i++ )
        {
                frac_vis = 0.0;

                if ( result.hit_fraction.m128_f32[i] < 1.0 - EQUAL_EPSILON )
                {
                        RayTraceGeometry *geom = scene->get_geometry( result.geom_id.m128_u32[i] );
                        //if ( geom == nullptr )
                        //{
                        //        std::cout << "Null trace geom:\n"
                        //                << "\thit fraction: " << result.hit_fraction.m128_f32[i] << "\n"
                        //                << "\thit?: " << result.hit.m128_i32[i] << "\n";
                        //}
                        contents = geom->get_mask().get_word();
                }
                else
                {
                        contents = CONTENTS_EMPTY;
                }
                if ( ( contents & contents_mask ) != 0 )
                {
                        frac_vis = 1.0;
                }
                
                *fraction4 = SetComponentSIMD( *fraction4, i, frac_vis );
        }
}

dface_t *RADTrace::get_dface( const RayTraceHitResult &result )
{
        int geomidx = dface_lookup.find( result.geom_id );
        if ( geomidx == -1 )
                return nullptr;

        //const PrimID2dface &prim2dface = dface_lookup.get_data( geomidx );
        //int primidx = prim2dface.find( result.prim_id );
        //if ( primidx == -1 )
        //        return nullptr;

        return dface_lookup.get_data( geomidx );
}