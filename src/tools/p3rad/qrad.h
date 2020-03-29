#ifndef HLRAD_H__
#define HLRAD_H__

#if _MSC_VER >= 1000
#pragma once
#endif

#include "cmdlib.h"
#include "messages.h"
#include "win32fix.h"
#include "log.h"
#include "hlassert.h"
#include "mathlib.h"
#include "bspfile.h"
#include "winding.h"
#include "scriplib.h"
#include "threads.h"
#include "blockmem.h"
#include "filelib.h"
#include "winding.h"
#include "cmdlinecfg.h"
#include "mathlib/ssemath.h"
#include "lights.h"

#include <pnmImage.h>

#ifdef _WIN32
#pragma warning(disable: 4142 4028)
#include <io.h>
#pragma warning(default: 4142 4028)
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef STDC_HEADERS
#include <ctype.h>
#endif

#ifdef _WIN32
#include <direct.h>
#endif

#define DEFAULT_FASTMODE			false
#define DEFAULT_METHOD eMethodSparseVismatrix
#define DEFAULT_LERP_ENABLED        true
#define DEFAULT_FADE                1.0
#define DEFAULT_BOUNCE              8
#define DEFAULT_DUMPPATCHES         false
#define DEFAULT_AMBIENT_RED         0.0
#define DEFAULT_AMBIENT_GREEN       0.0
#define DEFAULT_AMBIENT_BLUE        0.0
// 188 is the fullbright threshold for Goldsrc, regardless of the brightness and gamma settings in the graphic options.
// However, hlrad can only control the light values of each single light style. So the final in-game brightness may exceed 188 if you have set a high value in the "custom appearance" of the light, or if the face receives light from different styles.
#define DEFAULT_LIMITTHRESHOLD		255.0
#define DEFAULT_TEXSCALE            true
#define DEFAULT_CHOP                64.0
#define DEFAULT_TEXCHOP             32.0
#define DEFAULT_LIGHTSCALE          1.0 //1.0 //vluzacn
#define DEFAULT_DLIGHT_THRESHOLD	0.1
#define DEFAULT_DLIGHT_SCALE        2.0 //2.0 //vluzacn
#define DEFAULT_SMOOTHING_VALUE     45.0
#define DEFAULT_SMOOTHING2_VALUE	-1.0
#define DEFAULT_INCREMENTAL         false


// ------------------------------------------------------------------------
// Changes by Adam Foster - afoster@compsoc.man.ac.uk

// superseded by DEFAULT_COLOUR_LIGHTSCALE_*

// superseded by DEFAULT_COLOUR_GAMMA_*
// ------------------------------------------------------------------------

#define DEFAULT_INDIRECT_SUN        1.0
#define DEFAULT_EXTRA               false
#define DEFAULT_SKY_LIGHTING_FIX    true
#define DEFAULT_CIRCUS              false
#define DEFAULT_CORING				0.00
#define DEFAULT_SUBDIVIDE           true
#define DEFAULT_CHART               false
#define DEFAULT_INFO                true
#define DEFAULT_ALLOW_SPREAD		true

// ------------------------------------------------------------------------
// Changes by Adam Foster - afoster@compsoc.man.ac.uk

// Brian: just use 1.0 for gamma in rad, we can adjust it ourselves in game
#define DEFAULT_GAMMA 1.0
#define DEFAULT_COLOUR_GAMMA_RED		DEFAULT_GAMMA
#define DEFAULT_COLOUR_GAMMA_GREEN		DEFAULT_GAMMA
#define DEFAULT_COLOUR_GAMMA_BLUE		DEFAULT_GAMMA

#define DEFAULT_COLOUR_LIGHTSCALE_RED		1.0 //1.0 //vluzacn
#define DEFAULT_COLOUR_LIGHTSCALE_GREEN		1.0 //1.0 //vluzacn
#define DEFAULT_COLOUR_LIGHTSCALE_BLUE		1.0 //1.0 //vluzacn

#define DEFAULT_COLOUR_JITTER_HACK_RED		0.0
#define DEFAULT_COLOUR_JITTER_HACK_GREEN	0.0
#define DEFAULT_COLOUR_JITTER_HACK_BLUE		0.0

#define DEFAULT_JITTER_HACK_RED			0.0
#define DEFAULT_JITTER_HACK_GREEN		0.0
#define DEFAULT_JITTER_HACK_BLUE		0.0



// ------------------------------------------------------------------------

// O_o ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Changes by Jussi Kivilinna <hullu@unitedadmins.com> [http://hullu.xtragaming.com/]
// Transparency light support for bounced light(transfers) is extreamly slow 
// for 'vismatrix' and 'sparse' atm. 
// Only recommended to be used with 'nomatrix' mode
#define DEFAULT_CUSTOMSHADOW_WITH_BOUNCELIGHT false

// RGB Transfers support for HLRAD .. to be used with -customshadowwithbounce
#define DEFAULT_RGB_TRANSFERS false
// o_O ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define DEFAULT_TRANSTOTAL_HACK 0.2 //0.5 //vluzacn
#define DEFAULT_MINLIGHT 0
#define DEFAULT_TRANSFER_COMPRESS_TYPE FLOAT16
#define DEFAULT_RGBTRANSFER_COMPRESS_TYPE VECTOR32
#define DEFAULT_SOFTSKY true
#define DEFAULT_TRANSLUCENTDEPTH 2.0f
#define DEFAULT_NOTEXTURES false
#define DEFAULT_TEXREFLECTGAMMA 2.2f // 2.0(texgamma cvar) / 2.5 (gamma cvar) * 2.2 (screen gamma) = 1.76
#define DEFAULT_TEXREFLECTSCALE 1.0f // arbitrary (This is lower than 1.0, because textures are usually brightened in order to look better in Goldsrc. Textures are made brightened because Goldsrc is only able to darken the texture when combining the texture with the lightmap.)
#define DEFAULT_BLUR 1.0 // classic lighting is equivalent to "-blur 1.0"
#define DEFAULT_NOEMITTERRANGE false
#define DEFAULT_BLEEDFIX true
#define DEFAULT_TEXLIGHTGAP 0.0

#define		TRANSFER_EPSILON		0.0000001


#ifdef _WIN32
#define DEFAULT_ESTIMATE    false
#endif
#ifdef SYSTEM_POSIX
#define DEFAULT_ESTIMATE    true
#endif

// Ideally matches what is in the FGD :)
#define SPAWNFLAG_NOBLEEDADJUST    (1 << 0)

// DEFAULT_HUNT_OFFSET is how many units in front of the plane to place the samples
// Unit of '1' causes the 1 unit crate trick to cause extra shadows
#define DEFAULT_HUNT_OFFSET 0.5
// DEFAULT_HUNT_SIZE number of iterations (one based) of radial search in HuntForWorld
#define DEFAULT_HUNT_SIZE   11
// DEFAULT_HUNT_SCALE amount to grow from origin point per iteration of DEFAULT_HUNT_SIZE in HuntForWorld
#define DEFAULT_HUNT_SCALE 0.1
#define DEFAULT_EDGE_WIDTH 0.8

#define PATCH_HUNT_OFFSET 0.5 //--vluzacn
#define HUNT_WALL_EPSILON (3 * ON_EPSILON) // place sample at least this distance away from any wall //--vluzacn

#define MINIMUM_PATCH_DISTANCE ON_EPSILON
#define ACCURATEBOUNCE_THRESHOLD 4.0 // If the receiver patch is closer to emitter patch than EXACTBOUNCE_THRESHOLD * emitter_patch->radius, calculate the exact visibility amount.
#define ACCURATEBOUNCE_DEFAULT_SKYLEVEL 5 // sample 1026 normals


#define ALLSTYLES 64 // HL limit. //--vluzacn

#define BOGUS_RANGE 131072

typedef struct
{
        vec_t v[4][3];
}
matrix_t;

// a 4x4 matrix that represents the following transformation (see the ApplyMatrix function)
//
//  / X \    / v[0][0] v[1][0] v[2][0] v[3][0] \ / X \.
//  | Y | -> | v[0][1] v[1][1] v[2][1] v[3][1] | | Y |
//  | Z |    | v[0][2] v[1][2] v[2][2] v[3][2] | | Z |
//  \ 1 /    \    0       0       0       1    / \ 1 /

//
// LIGHTMAP.C STUFF
//

extern Winding *WindingFromFace( dface_t *f );

INLINE byte PVSCheck( const byte *pvs, int leaf )
{
        leaf -= 1;

        if ( leaf >= 0 )
        {
                return pvs[leaf >> 3] & ( 1 << ( leaf & 7 ) );
        }
        
        return 1;
}

typedef struct lightvalue_s
{
        lightvalue_s()
        {
                Zero();
        }

        LVector3 light;
        float direct_sun_amt;

        float &operator []( int n )
        {
                return light[n];
        }

        INLINE void Zero()
        {
                light.set( 0.0, 0.0, 0.0 );
                direct_sun_amt = 0.0;
        }

        INLINE void Scale( float scale )
        {
                light *= scale;
                direct_sun_amt *= scale;
        }

        INLINE void AddWeighted( lightvalue_s &src, float weight )
        {
                light += weight * src.light;
                direct_sun_amt += weight * src.direct_sun_amt;
        }

        INLINE void AddWeighted( LVector3 &src, float weight )
        {
                light += weight * src;
        }

        INLINE float Intensity() const
        {
                return light[0] + light[1] + light[2];
        }

        INLINE void AddLight( float amt, const LVector3 &color, float sun_amt = 0.0 )
        {
                VectorMA( light, amt, color, light );
                direct_sun_amt += sun_amt;
        }

        INLINE void AddLight( lightvalue_s &src )
        {
                light += src.light;
                direct_sun_amt += src.direct_sun_amt;
        }


} lightvalue_t;

typedef struct
{
        lightvalue_t light[NUM_BUMP_VECTS + 1];

        lightvalue_t &operator []( int n )
        {
                return light[n];
        }

} bumpsample_t;

typedef struct
{
        vec3_t norm[NUM_BUMP_VECTS + 1];
} bumpnormal_t;

#define MAX_COMPRESSED_TRANSFER_INDEX_SIZE ((1 << 12) - 1)

#define	MAX_PATCHES	(65535*16) // limited by transfer_index_t
#define MAX_VISMATRIX_PATCHES 65535
#define MAX_SPARSE_VISMATRIX_PATCHES MAX_PATCHES

struct transfer_t
{
        int patch;
        float transfer;
};

struct patch_t
{
        Winding *winding;
        LVector3 mins, maxs, face_mins, face_maxs;
        LVector3 origin;
        dplane_t *plane; // plane (corrected for facing)
        unsigned short iteration_key;

        // these are packed into one dword
        unsigned int normal_major_axis : 2; // major axis of base face normal
        unsigned int sky : 1;
        unsigned int bumped : 1;
        unsigned int pad : 28;

        LVector3 normal; // adjusted for phong shading

        float plane_dist;

        float chop;
        float luxscale;
        float scale[2];

        bumpsample_t totallight;
        LVector3 baselight;
        float basearea;

        LVector3 directlight;
        float area;

        LVector3 reflectivity;

        LVector3 samplelight;
        float samplearea;
        int facenum;
        int leafnum;
        int parent;
        int child1;
        int child2;

        int next;
        int nextparent;
        int nextclusterchild;

        int numtransfers;
        transfer_t *transfers;

        short indices[3];

        bool translucent_b;
        float emitter_range;
};

extern pvector<patch_t> g_patches;
extern pvector<int> g_face_patches;
extern pvector<int> g_face_parents;
extern pvector<int> g_cluster_children;


struct SSE_sampleLightOutput_t
{
        fltx4 dot[NUM_BUMP_VECTS + 1];
        fltx4 falloff;
        fltx4 sun_amount;
};

struct SSE_sampleLightInput_t
{
        directlight_t *dl;
        int facenum;
        FourVectors pos;
        FourVectors *normals;
        int normal_count;
        int thread;
        int lightflags;
        int epsilon;
};

extern vector_string g_multifiles; // for loading textures and static props
extern string g_mfincludefile;

typedef struct
{
        char name[MAX_TEXTURE_NAME]; // not always same with the name in texdata
                                     //int width, height;
                                     //byte *canvas; //[height][width]
                                     //byte palette[256][3];
        PNMImage *image;
        vec3_t reflectivity;
        PNMImage *bump;
} radtexture_t;
extern int g_numtextures;
extern radtexture_t *g_textures;
extern void LoadTextures();

//
// qrad globals
//

extern int             leafparents[MAX_MAP_LEAFS];
extern int             nodeparents[MAX_MAP_NODES];

extern entity_t* g_face_entity[MAX_MAP_FACES];
extern vec3_t   g_face_offset[MAX_MAP_FACES];              // for models with origins
extern vec3_t   g_face_centroids[MAX_MAP_EDGES];

extern float    g_lightscale;
extern float    g_dlight_threshold;
extern float    g_coring;

#include <simpleHashMap.h>

struct radtimer_t
{
        string operation;
        double total;
        
        SimpleHashMap<string, double, string_hash> subops;

        void record_subop( const string &op, double time )
        {
                ThreadLock();

                if ( subops.find( op ) == -1 )
                {
                        subops[op] = time;
                }
                else
                {
                        subops[op] += time;
                }

                total += time;

                ThreadUnlock();
        }

        void report()
        {
                ThreadLock();

                std::cout << "Operation " << operation << " (Total " << total << " seconds):" << std::endl;
                for ( size_t i = 0; i < subops.size(); i++ )
                {
                        std::cout << "\tSub-operation " << subops.get_key( i ) << " took "
                                << subops.get_data( i ) << " seconds (" << subops.get_data( i ) / total << "%)" << std::endl;
                }

                ThreadUnlock();
        }
};

extern pvector<radtimer_t> g_radtimers;

INLINE void RecordRadTimerOp( const string &op, const string &subop, double time )
{
        radtimer_t *found = nullptr;
        for ( size_t i = 0; i < g_radtimers.size(); i++ )
        {
                if ( g_radtimers[i].operation == op )
                {
                        found = &g_radtimers[i];
                }
        }

        if ( !found )
        {
                radtimer_t timer;
                timer.operation = op;
                timer.total = 0.0;
                size_t idx = g_radtimers.size();
                g_radtimers.push_back( timer );
                found = &g_radtimers[idx];
        }

        found->record_subop( subop, time );
}

#define StartRadTimer(op, subop) double _##op##_##subop##_begin = ClockObject::get_global_clock()->get_real_time();
#define StopRadTimer(op, subop) \
double _##op##_##subop##_end = ClockObject::get_global_clock()->get_real_time();\
RecordRadTimerOp(#op, #subop, _##op##_##subop##_end - _##op##_##subop##_begin);

INLINE void ReportRadTimers()
{
        for ( size_t i = 0; i < g_radtimers.size(); i++ )
        {
                g_radtimers[i].report();
        }
}

//==============================================

extern bool		g_fastmode;
extern bool     g_extra;
extern float   g_ambient[3];
extern float    g_direct_scale;
extern float	g_limitthreshold;
extern bool		g_drawoverload;
extern unsigned g_numbounce;
extern float    g_qgamma;
extern float    g_indirect_sun;
extern float    g_smoothing_threshold;
extern float    g_smoothing_value;
extern float g_smoothing_threshold_2;
extern float g_smoothing_value_2;
extern float *g_smoothvalues; //[numtexref]
extern bool     g_estimate;
extern char     g_source[_MAX_PATH];
extern float    g_fade;
extern bool     g_incremental;
extern bool     g_circus;
extern bool		g_allow_spread;
extern bool     g_sky_lighting_fix;
extern float    g_chop;    // Chop value for normal textures
extern float    g_texchop; // Chop value for texture lights
extern float    g_minchop;
extern float    g_maxchop;


                                                 // ------------------------------------------------------------------------
                                                 // Changes by Adam Foster - afoster@compsoc.man.ac.uk

extern vec3_t	g_colour_qgamma;
extern vec3_t	g_colour_lightscale;

extern vec3_t	g_colour_jitter_hack;
extern vec3_t	g_jitter_hack;

extern int g_extrapasses;

// ------------------------------------------------------------------------


extern bool	g_customshadow_with_bouncelight;
extern bool	g_rgb_transfers;
extern const vec3_t vec3_one;

extern float g_transtotal_hack;
extern unsigned char g_minlight;
extern bool g_softsky;
extern bool g_drawpatch;
extern bool g_drawsample;
extern vec3_t g_drawsample_origin;
extern vec_t g_drawsample_radius;
extern bool g_drawedge;
extern bool g_drawlerp;
extern bool g_drawnudge;
extern float g_corings[ALLSTYLES];
extern int stylewarningcount; // not thread safe
extern int stylewarningnext; // not thread safe
extern vec3_t *g_translucenttextures;
extern vec_t g_translucentdepth;
extern vec3_t *g_lightingconeinfo; //[numtexref]; X component = power, Y component = scale, Z component = nothing
extern bool g_notextures;
extern vec_t g_texreflectgamma;
extern vec_t g_texreflectscale;
extern vec_t g_blur;
extern bool g_noemitterrange;
extern bool g_bleedfix;
extern vec_t g_maxdiscardedlight;
extern vec3_t g_maxdiscardedpos;
extern vec_t g_texlightgap;
extern float g_skysamplescale;

extern void     DetermineLightmapMemory();
extern void     PairEdges();
extern void     BuildFacelights( int facenum );
extern void     PrecompLightmapOffsets();
extern void		ReduceLightmap();
//extern void     FinalLightFace( int facenum );
extern void		ScaleDirectLights(); // run before AddPatchLights
extern void		CreateFacelightDependencyList(); // run before AddPatchLights
extern void		FreeFacelightDependencyList();
extern void MakeTransfer( int patchidx1, int patchidx2, transfer_t *all_transfers );

extern void     GetPhongNormal( int facenum, const LVector3 &spot, LVector3 &phongnormal );

typedef bool( *funcCheckVisBit ) ( unsigned, unsigned
                                   , vec3_t&
                                   , unsigned int&
                                   );
extern funcCheckVisBit g_CheckVisBit;

// qradutil.c
extern vec_t    PatchPlaneDist( const patch_t* const patch );
extern dleaf_t* PointInLeafD( const vec3_t &point );
extern dleaf_t* PointInLeaf( const float *point );
extern dleaf_t* PointInLeaf( const LVector3 &point );
extern void     MakeBackplanes();
extern const dplane_t* getPlaneFromFace( const dface_t* const face );
extern const dplane_t* getPlaneFromFaceNumber( unsigned int facenum );
extern void     getAdjustedPlaneFromFaceNumber( unsigned int facenum, dplane_t* plane );
extern void		ApplyMatrix( const matrix_t &m, const vec3_t in, vec3_t &out );
extern void		ApplyMatrixOnPlane( const matrix_t &m_inverse, const vec3_t in_normal, vec_t in_dist, vec3_t &out_normal, vec_t &out_dist );
extern void		MultiplyMatrix( const matrix_t &m_left, const matrix_t &m_right, matrix_t &m );
extern matrix_t	MultiplyMatrix( const matrix_t &m_left, const matrix_t &m_right );
extern void		MatrixForScale( const vec3_t center, vec_t scale, matrix_t &m );
extern matrix_t	MatrixForScale( const vec3_t center, vec_t scale );
extern vec_t	CalcMatrixSign( const matrix_t &m );
extern void		TranslateWorldToTex( int facenum, matrix_t &m );
extern bool		InvertMatrix( const matrix_t &m, matrix_t &m_inverse );

// transfers.c
extern size_t   g_total_transfer;
extern void     MakeScales( int patch, transfer_t *all_transfers );

// mathutil.c
extern bool     intersect_linesegment_plane( const dplane_t* const plane, const vec_t* const p1, const vec_t* const p2, vec3_t point );
extern void     plane_from_points( const vec3_t p1, const vec3_t p2, const vec3_t p3, dplane_t* plane );
extern bool     point_in_winding( const Winding& w, const dplane_t& plane, const vec_t* point
                                  , vec_t epsilon = 0.0
);
extern bool     point_in_winding_noedge( const Winding& w, const dplane_t& plane, const vec_t* point, vec_t width );
extern void     snap_to_winding( const Winding& w, const dplane_t& plane, vec_t* point );
extern vec_t	snap_to_winding_noedge( const Winding& w, const dplane_t& plane, vec_t* point, vec_t width, vec_t maxmove );
extern void     SnapToPlane( const dplane_t* const plane, vec_t* const point, vec_t offset );

#endif //HLRAD_H__
