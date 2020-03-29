/*

R A D I O S I T Y    -aka-    R A D

Code based on original code from Valve Software,
Modified by Sean "Zoner" Cavanaugh (seanc@gearboxsoftware.com) with permission.
Modified by Tony "Merl" Moore (merlinis@bigpond.net.au) [AJM]
Modified by Brian Lach (brianlach72@gmail.com) for use with the Panda3D game engine.

==================================================================================

Pieces of RAD:

Direct lights:  light sources in the map (point lights, spot lights, etc)
Patches:        light emitting rectangles (used for bouncing light)
Samples:        points on a face to calculate a brightness value for (lightmap sampling points)
Luxels:         pixels in a lightmap, coorespond to a sample

===================================================================================

A simple summary of RAD goes as follows:

* Build a list of all lights in the map.
* Make a single patch for each face.
* Recursively chop up (subdivide) the face patches to meet a certain size criteria.

* Then for each face
        - Determine width and height of lightmap (depending on lightmap scale, face extents)
        - Chop up the face to build a sample for each S,T position
        - Build list of luxels (hold a world-space position and a lighting value) for each S,T position
        - For each sample on the face, calculate a brightness value based on contributions from the direct lights,
          store on the cooresponding luxel.
        - Average up the luxels to determine a brightness value for each patch on the face (for the radiosity pass)

* Determine which patches can see each other
* For each patch, determine how much light would transfer from each visible patch to itself

* Bounce the light, do as many passes until scene stabilizes, store the bounced light into a separate structure

* For each luxel on each face, add the bounced light onto the direct light and store into final map format

*/

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <vector>
#include <string>

#include "qrad.h"
#include "radstaticprop.h"
#include "radial.h"
#include "leaf_ambient_lighting.h"
#include "lights.h"
#include "vismat.h"
#include "trace.h"
//#include "clhelper.h"
#include <virtualFileSystem.h>
#include <simpleHashMap.h>
#include <load_prc_file.h>
#include <pStatClient.h>
#include <pStatTimer.h>

static PStatCollector radworld_collector( "RadWorld" );
static PStatCollector bfl_collector( "RadWorld:BuildFacelights" );

/*
* NOTES
* -----
* every surface must be divided into at least two g_patches each axis
*/

bool			g_fastmode = DEFAULT_FASTMODE;
typedef enum
{
        eMethodVismatrix,
        eMethodSparseVismatrix,
        eMethodNoVismatrix
}
eVisMethods;

eVisMethods		g_method = DEFAULT_METHOD;

float           g_fade = DEFAULT_FADE;

entity_t*       g_face_entity[MAX_MAP_FACES];
entity_t*		g_face_texlights[MAX_MAP_FACES];

pvector<patch_t> g_patches;
pvector<int> g_face_patches;
pvector<int> g_face_parents;
pvector<int> g_cluster_children;

size_t g_total_transfer = 0;

float g_minchop = 4;
float g_maxchop = 4;

static pvector<LVector3> emitlight;
static pvector<bumpsample_t> addlight;

vector_string	g_multifiles;
string		g_mfincludefile;

vec3_t          g_face_offset[MAX_MAP_FACES];              // for rotating bmodels

float           g_direct_scale = DEFAULT_DLIGHT_SCALE;
float           g_skysamplescale = 1.0;
int             g_extrapasses = 4;

unsigned        g_numbounce = 100; // max number of bounces

static bool     g_dumppatches = DEFAULT_DUMPPATCHES;

float          g_ambient[3] = { DEFAULT_AMBIENT_RED, DEFAULT_AMBIENT_GREEN, DEFAULT_AMBIENT_BLUE };
float			g_limitthreshold = DEFAULT_LIMITTHRESHOLD;
bool			g_drawoverload = false;

float           g_lightscale = DEFAULT_LIGHTSCALE;
float           g_dlight_threshold = DEFAULT_DLIGHT_THRESHOLD;  // was DIRECT_LIGHT constant

char            g_source[_MAX_PATH] = "";

char            g_vismatfile[_MAX_PATH] = "";
bool            g_incremental = DEFAULT_INCREMENTAL;
float           g_indirect_sun = DEFAULT_INDIRECT_SUN;
bool            g_extra = DEFAULT_EXTRA;
bool            g_texscale = DEFAULT_TEXSCALE;

float           g_smoothing_threshold;
float           g_smoothing_value = DEFAULT_SMOOTHING_VALUE;
float			g_smoothing_threshold_2;
float			g_smoothing_value_2 = DEFAULT_SMOOTHING2_VALUE;

bool            g_circus = DEFAULT_CIRCUS;
bool			g_allow_spread = DEFAULT_ALLOW_SPREAD;

pvector<radtimer_t> g_radtimers;

// --------------------------------------------------------------------------
// Changes by Adam Foster - afoster@compsoc.man.ac.uk
vec3_t		g_colour_qgamma = { DEFAULT_COLOUR_GAMMA_RED, DEFAULT_COLOUR_GAMMA_GREEN, DEFAULT_COLOUR_GAMMA_BLUE };
vec3_t		g_colour_lightscale = { DEFAULT_COLOUR_LIGHTSCALE_RED, DEFAULT_COLOUR_LIGHTSCALE_GREEN, DEFAULT_COLOUR_LIGHTSCALE_BLUE };
vec3_t		g_colour_jitter_hack = { DEFAULT_COLOUR_JITTER_HACK_RED, DEFAULT_COLOUR_JITTER_HACK_GREEN, DEFAULT_COLOUR_JITTER_HACK_BLUE };
vec3_t		g_jitter_hack = { DEFAULT_JITTER_HACK_RED, DEFAULT_JITTER_HACK_GREEN, DEFAULT_JITTER_HACK_BLUE };
// --------------------------------------------------------------------------

bool		g_customshadow_with_bouncelight = DEFAULT_CUSTOMSHADOW_WITH_BOUNCELIGHT;
bool		g_rgb_transfers = DEFAULT_RGB_TRANSFERS;

float		g_transtotal_hack = DEFAULT_TRANSTOTAL_HACK;
unsigned char g_minlight = DEFAULT_MINLIGHT;
bool g_softsky = DEFAULT_SOFTSKY;
bool g_notextures = DEFAULT_NOTEXTURES;
vec_t g_texreflectgamma = DEFAULT_TEXREFLECTGAMMA;
vec_t g_texreflectscale = DEFAULT_TEXREFLECTSCALE;
bool g_bleedfix = DEFAULT_BLEEDFIX;
bool g_drawpatch = false;
bool g_drawsample = false;
vec3_t g_drawsample_origin = { 0,0,0 };
vec_t g_drawsample_radius = 0;
bool g_drawedge = false;
bool g_drawlerp = false;
bool g_drawnudge = false;

// Cosine of smoothing angle(in radians)
float           g_coring = DEFAULT_CORING;                 // Light threshold to force to blackness(minimizes lightmaps)
bool            g_chart = DEFAULT_CHART;
bool            g_estimate = DEFAULT_ESTIMATE;
bool            g_info = DEFAULT_INFO;


// Patch creation and subdivision criteria
bool            g_subdivide = DEFAULT_SUBDIVIDE;
float           g_chop = DEFAULT_CHOP;
float           g_texchop = DEFAULT_TEXCHOP;

float			g_corings[ALLSTYLES];
vec3_t*			g_translucenttextures = NULL;
vec_t			g_translucentdepth = DEFAULT_TRANSLUCENTDEPTH;
vec_t			g_blur = DEFAULT_BLUR;
bool			g_noemitterrange = DEFAULT_NOEMITTERRANGE;
vec_t			g_texlightgap = DEFAULT_TEXLIGHTGAP;

// Misc
int             leafparents[MAX_MAP_LEAFS];
int             nodeparents[MAX_MAP_NODES];
int				stylewarningcount = 0;
int				stylewarningnext = 1;
vec_t g_maxdiscardedlight = 0;
vec3_t g_maxdiscardedpos = { 0, 0, 0 };

static NodePath g_rtroot( "RTRoot" );

// =====================================================================================
//  GetParamsFromEnt
//      this function is called from parseentity when it encounters the 
//      info_compile_parameters entity. each tool should have its own version of this
//      to handle its own specific settings.
// =====================================================================================
void            GetParamsFromEnt( entity_t* mapent )
{
        int     iTmp;
        float   flTmp;
        char    szTmp[256]; //lightdata
        const char* pszTmp;

        Log( "\nCompile Settings detected from info_compile_parameters entity\n" );

        // lightdata(string) : "Lighting Data Memory" : "8192"
        iTmp = IntForKey( mapent, "lightdata" ) * 1024; //lightdata
        if ( iTmp > g_max_map_lightdata ) //--vluzacn
        {
                g_max_map_lightdata = iTmp;
                sprintf_s( szTmp, "%i", g_max_map_lightdata );
                Log( "%30s [ %-9s ]\n", "Lighting Data Memory", szTmp );
        }

        // verbose(choices) : "Verbose compile messages" : 0 = [ 0 : "Off" 1 : "On" ]
        iTmp = IntForKey( mapent, "verbose" );
        if ( iTmp == 1 )
        {
                g_verbose = true;
        }
        else if ( iTmp == 0 )
        {
                g_verbose = false;
        }
        Log( "%30s [ %-9s ]\n", "Compile Option", "setting" );
        Log( "%30s [ %-9s ]\n", "Verbose Compile Messages", g_verbose ? "on" : "off" );

        // estimate(choices) :"Estimate Compile Times?" : 0 = [ 0: "Yes" 1: "No" ]
        if ( IntForKey( mapent, "estimate" ) )
        {
                g_estimate = true;
        }
        else
        {
                g_estimate = false;
        }
        Log( "%30s [ %-9s ]\n", "Estimate Compile Times", g_estimate ? "on" : "off" );

        // priority(choices) : "Priority Level" : 0 = [	0 : "Normal" 1 : "High"	-1 : "Low" ]
        if ( !strcmp( ValueForKey( mapent, "priority" ), "1" ) )
        {
                g_threadpriority = TP_high;
                Log( "%30s [ %-9s ]\n", "Thread Priority", "high" );
        }
        else if ( !strcmp( ValueForKey( mapent, "priority" ), "-1" ) )
        {
                g_threadpriority = TP_low;
                Log( "%30s [ %-9s ]\n", "Thread Priority", "low" );
        }

        // bounce(integer) : "Number of radiosity bounces" : 0 
        iTmp = IntForKey( mapent, "bounce" );
        if ( iTmp )
        {
                g_numbounce = abs( iTmp );
                Log( "%30s [ %-9s ]\n", "Number of radiosity bounces", ValueForKey( mapent, "bounce" ) );
        }

        iTmp = IntForKey( mapent, "customshadowwithbounce" );
        if ( iTmp )
        {
                g_customshadow_with_bouncelight = true;
                Log( "%30s [ %-9s ]\n", "Custom Shadow with Bounce Light", ValueForKey( mapent, "customshadowwithbounce" ) );
        }
        iTmp = IntForKey( mapent, "rgbtransfers" );
        if ( iTmp )
        {
                g_rgb_transfers = true;
                Log( "%30s [ %-9s ]\n", "RGB Transfers", ValueForKey( mapent, "rgbtransfers" ) );
        }

        // ambient(string) : "Ambient world light (0.0 to 1.0, R G B)" : "0 0 0" 
        //vec3_t          g_ambient = { DEFAULT_AMBIENT_RED, DEFAULT_AMBIENT_GREEN, DEFAULT_AMBIENT_BLUE };
        pszTmp = ValueForKey( mapent, "ambient" );
        if ( pszTmp )
        {
                float red = 0, green = 0, blue = 0;
                if ( sscanf( pszTmp, "%f %f %f", &red, &green, &blue ) )
                {
                        if ( red < 0 || red > 1 || green < 0 || green > 1 || blue < 0 || blue > 1 )
                        {
                                Error( "info_compile_parameters: Ambient World Light (ambient) all 3 values must be within the range of 0.0 to 1.0\n"
                                       "Parsed values:\n"
                                       "    red [ %1.3f ] %s\n"
                                       "  green [ %1.3f ] %s\n"
                                       "   blue [ %1.3f ] %s\n"
                                       , red, ( red   < 0 || red   > 1 ) ? "OUT OF RANGE" : ""
                                       , green, ( green < 0 || green > 1 ) ? "OUT OF RANGE" : ""
                                       , blue, ( blue  < 0 || blue  > 1 ) ? "OUT OF RANGE" : "" );
                        }

                        if ( red == 0 && green == 0 && blue == 0 )
                        {
                        } // dont bother setting null values
                        else
                        {
                                g_ambient[0] = red * 128;
                                g_ambient[1] = green * 128;
                                g_ambient[2] = blue * 128;
                                Log( "%30s [ %1.3f %1.3f %1.3f ]\n", "Ambient world light (R G B)", red, green, blue );
                        }
                }
                else
                {
                        Error( "info_compile_parameters: Ambient World Light (ambient) has unrecognised value\n"
                               "This keyvalue accepts 3 numeric values from 0.000 to 1.000, use \"0 0 0\" if in doubt" );
                }
        }

        // smooth(integer) : "Smoothing threshold (in degrees)" : 0 
        flTmp = FloatForKey( mapent, "smooth" );
        if ( flTmp )
        {
                /*g_smoothing_threshold = flTmp;*/
                g_smoothing_threshold = cos( g_smoothing_value * ( Q_PI / 180.0 ) ); // --vluzacn
                Log( "%30s [ %-9s ]\n", "Smoothing threshold", ValueForKey( mapent, "smooth" ) );
        }

        // dscale(integer) : "Direct Lighting Scale" : 1 
        flTmp = FloatForKey( mapent, "dscale" );
        if ( flTmp )
        {
                g_direct_scale = flTmp;
                Log( "%30s [ %-9s ]\n", "Direct Lighting Scale", ValueForKey( mapent, "dscale" ) );
        }

        // chop(integer) : "Chop Size" : 64 
        iTmp = IntForKey( mapent, "chop" );
        if ( iTmp )
        {
                g_chop = iTmp;
                Log( "%30s [ %-9s ]\n", "Chop Size", ValueForKey( mapent, "chop" ) );
        }

        // texchop(integer) : "Texture Light Chop Size" : 32 
        flTmp = FloatForKey( mapent, "texchop" );
        if ( flTmp )
        {
                g_texchop = flTmp;
                Log( "%30s [ %-9s ]\n", "Texture Light Chop Size", ValueForKey( mapent, "texchop" ) );
        }

        /*
        hlrad(choices) : "HLRAD" : 0 =
        [
        0 : "Off"
        1 : "Normal"
        2 : "Extra"
        ]
        */
        iTmp = IntForKey( mapent, "hlrad" );
        if ( iTmp == 0 )
        {
                Fatal( assume_TOOL_CANCEL,
                       "%s flag was not checked in info_compile_parameters entity, execution of %s cancelled", g_Program, g_Program );
                CheckFatal();
        }
        else if ( iTmp == 1 )
        {
                g_extra = false;
        }
        else if ( iTmp == 2 )
        {
                g_extra = true;
        }
        Log( "%30s [ %-9s ]\n", "Extra RAD", g_extra ? "on" : "off" );

        /*
        sparse(choices) : "Vismatrix Method" : 2 =
        [
        0 : "No Vismatrix"
        1 : "Sparse Vismatrix"
        2 : "Normal"
        ]
        */
        iTmp = IntForKey( mapent, "sparse" );
        if ( iTmp == 1 )
        {
                g_method = eMethodSparseVismatrix;
        }
        else if ( iTmp == 0 )
        {
                g_method = eMethodNoVismatrix;
        }
        else if ( iTmp == 2 )
        {
                g_method = eMethodVismatrix;
        }
        Log( "%30s [ %-9s ]\n", "Sparse Vismatrix", g_method == eMethodSparseVismatrix ? "on" : "off" );
        Log( "%30s [ %-9s ]\n", "NoVismatrix", g_method == eMethodNoVismatrix ? "on" : "off" );

        /*
        circus(choices) : "Circus RAD lighting" : 0 =
        [
        0 : "Off"
        1 : "On"
        ]
        */
        iTmp = IntForKey( mapent, "circus" );
        if ( iTmp == 0 )
        {
                g_circus = false;
        }
        else if ( iTmp == 1 )
        {
                g_circus = true;
        }

        Log( "%30s [ %-9s ]\n", "Circus Lighting Mode", g_circus ? "on" : "off" );

        ////////////////////
        Log( "\n" );
}

// =====================================================================================
//  MakeParents
//      blah
// =====================================================================================
static void     MakeParents( const int nodenum, const int parent )
{
        int             i;
        int             j;
        dnode_t*        node;

        nodeparents[nodenum] = parent;
        node = g_bspdata->dnodes + nodenum;

        for ( i = 0; i < 2; i++ )
        {
                j = node->children[i];
                if ( j < 0 )
                {
                        leafparents[-j - 1] = nodenum;
                }
                else
                {
                        MakeParents( j, nodenum );
                }
        }
}

// =====================================================================================
//
//    TEXTURE LIGHT VALUES
//
// =====================================================================================

// misc
typedef struct
{
        std::string     name;
        vec3_t          value;
        const char*     filename;
}
texlight_t;

static std::vector< texlight_t > s_texlights;
typedef std::vector< texlight_t >::iterator texlight_i;

// =====================================================================================
//  ReadLightFile
// =====================================================================================
static void     ReadLightFile( const char* const filename )
{
        FILE*           f;
        char            scan[MAXTOKEN];
        short           argCnt;
        unsigned int    file_texlights = 0;

        f = fopen( filename, "r" );
        if ( !f )
        {
                Warning( "Could not open texlight file %s", filename );
                return;
        }
        else
        {
                Log( "Reading texlights from '%s'\n", filename );
        }

        while ( fgets( scan, sizeof( scan ), f ) )
        {
                char*           comment;
                char            szTexlight[_MAX_PATH];
                vec_t           r, g, b, i = 1;

                comment = strstr( scan, "//" );
                if ( comment )
                {
                        // Newline and Null terminate the string early if there is a c++ style single line comment
                        comment[0] = '\n';
                        comment[1] = 0;
                }

                argCnt = sscanf( scan, "%s %f %f %f %f", szTexlight, &r, &g, &b, &i );

                if ( argCnt == 2 )
                {
                        // With 1+1 args, the R,G,B values are all equal to the first value
                        g = b = r;
                }
                else if ( argCnt == 5 )
                {
                        // With 1+4 args, the R,G,B values are "scaled" by the fourth numeric value i;
                        r *= i / 255.0;
                        g *= i / 255.0;
                        b *= i / 255.0;
                }
                else if ( argCnt != 4 )
                {
                        if ( strlen( scan ) > 4 )
                        {
                                Warning( "ignoring bad texlight '%s' in %s", scan, filename );
                        }
                        continue;
                }

                texlight_i it;
                for ( it = s_texlights.begin(); it != s_texlights.end(); it++ )
                {
                        if ( strcmp( it->name.c_str(), szTexlight ) == 0 )
                        {
                                if ( strcmp( it->filename, filename ) == 0 )
                                {
                                        Warning( "Duplication of texlight '%s' in file '%s'!", it->name.c_str(), it->filename );
                                }
                                else if ( it->value[0] != r || it->value[1] != g || it->value[2] != b )
                                {
                                        Warning( "Overriding '%s' from '%s' with '%s'!", it->name.c_str(), it->filename, filename );
                                }
                                else
                                {
                                        Warning( "Redundant '%s' def in '%s' AND '%s'!", it->name.c_str(), it->filename, filename );
                                }
                                s_texlights.erase( it );
                                break;
                        }
                }

                texlight_t      texlight;
                texlight.name = szTexlight;
                texlight.value[0] = r;
                texlight.value[1] = g;
                texlight.value[2] = b;
                texlight.filename = filename;
                file_texlights++;
                s_texlights.push_back( texlight );
        }
        fclose( f ); //--vluzacn
}

// =====================================================================================
//  LightForTexture
// =====================================================================================
static void     LightForTexture( const char* const name, LVector3 &result )
{
        texlight_i it;
        for ( it = s_texlights.begin(); it != s_texlights.end(); it++ )
        {
                if ( !strcasecmp( name, it->name.c_str() ) )
                {
                        VectorCopy( it->value, result );
                        return;
                }
        }
        VectorClear( result );
}


// =====================================================================================
//
//    MAKE FACES
//
// =====================================================================================

// =====================================================================================
//  BaseLightForFace
// =====================================================================================
static void     BaseLightForFace( const dface_t* const f, LVector3 &light, float *area, LVector3 &reflectivity )
{
        texinfo_t*      tx;
        texref_t*       tref;

        //
        // check for light emited by texture
        //
        tx = &g_bspdata->texinfo[f->texinfo];
        tref = &g_bspdata->dtexrefs[tx->texref];

        LightForTexture( tref->name, light );

        PNMImage *img = g_textures[tx->texref].image;
        *area = img->get_x_size() * img->get_y_size();

        VectorScale( g_textures[tx->texref].reflectivity, g_texreflectscale, reflectivity );

        // always keep this less than 1 or the solution will not converge
        for ( int i = 0; i < 3; i++ )
        {
                if ( reflectivity[i] > 0.99 )
                        reflectivity[i] = 0.99;
        }
}

// =====================================================================================
//  IsSpecial
// =====================================================================================
static bool     IsSpecial( const dface_t* const f )
{
        return g_bspdata->texinfo[f->texinfo].flags & TEX_SPECIAL;
}


// =====================================================================================
//
//    SUBDIVIDE PATCHES
//
// =====================================================================================

// =====================================================================================
//  MakePatchForFace
static float    totalarea = 0;
// =====================================================================================

float *g_smoothvalues; //[numtexref]
void ReadCustomSmoothValue()
{
        int num;
        int i, k;
        entity_t *mapent;
        epair_t *ep;

        num = g_bspdata->numtexrefs;
        g_smoothvalues = (float *)malloc( num * sizeof( vec_t ) );
        for ( i = 0; i < num; i++ )
        {
                g_smoothvalues[i] = g_smoothing_threshold;
        }
        for ( k = 0; k < g_bspdata->numentities; k++ )
        {
                mapent = &g_bspdata->entities[k];
                if ( strcmp( ValueForKey( mapent, "classname" ), "info_smoothvalue" ) )
                        continue;
                Developer( DEVELOPER_LEVEL_MESSAGE, "info_smoothvalue entity detected.\n" );
                for ( i = 0; i < num; i++ )
                {
                        const char *texname = ( &g_bspdata->dtexrefs[num] )->name;
                        for ( ep = mapent->epairs; ep; ep = ep->next )
                        {
                                if ( strcasecmp( ep->key, texname ) )
                                        continue;
                                if ( !strcasecmp( ep->key, "origin" ) )
                                        continue;
                                g_smoothvalues[i] = cos( atof( ep->value ) * ( Q_PI / 180.0 ) );
                                Developer( DEVELOPER_LEVEL_MESSAGE, "info_smoothvalue: %s = %f\n", texname, atof( ep->value ) );
                        }
                }
        }
}
void ReadTranslucentTextures()
{
        int num;
        int i, k;
        entity_t *mapent;
        epair_t *ep;

        num = g_bspdata->numtexrefs;
        g_translucenttextures = (vec3_t *)malloc( num * sizeof( vec3_t ) );
        for ( i = 0; i < num; i++ )
        {
                VectorClear( g_translucenttextures[i] );
        }
        for ( k = 0; k < g_bspdata->numentities; k++ )
        {
                mapent = &g_bspdata->entities[k];
                if ( strcmp( ValueForKey( mapent, "classname" ), "info_translucent" ) )
                        continue;
                Developer( DEVELOPER_LEVEL_MESSAGE, "info_translucent entity detected.\n" );
                for ( i = 0; i < num; i++ )
                {
                        const char *texname = ( &g_bspdata->dtexrefs[num] )->name;
                        for ( ep = mapent->epairs; ep; ep = ep->next )
                        {
                                if ( strcasecmp( ep->key, texname ) )
                                        continue;
                                if ( !strcasecmp( ep->key, "origin" ) )
                                        continue;
                                double r, g, b;
                                int count;
                                count = sscanf( ep->value, "%lf %lf %lf", &r, &g, &b );
                                if ( count == 1 )
                                {
                                        g = b = r;
                                }
                                else if ( count != 3 )
                                {
                                        Warning( "ignore bad translucent value '%s'", ep->value );
                                        continue;
                                }
                                if ( r < 0.0 || r > 1.0 || g < 0.0 || g > 1.0 || b < 0.0 || b > 1.0 )
                                {
                                        Warning( "translucent value should be 0.0-1.0" );
                                        continue;
                                }
                                g_translucenttextures[i][0] = r;
                                g_translucenttextures[i][1] = g;
                                g_translucenttextures[i][2] = b;
                                Developer( DEVELOPER_LEVEL_MESSAGE, "info_translucent: %s = %f %f %f\n", texname, r, g, b );
                        }
                }
        }
}
vec3_t *g_lightingconeinfo;//[numtexref]
static vec_t DefaultScaleForPower( vec_t power )
{
        vec_t scale;
        // scale = Pi / Integrate [2 Pi * Sin [x] * Cos[x] ^ power, {x, 0, Pi / 2}]
        scale = ( 1 + power ) / 2.0;
        return scale;
}
void ReadLightingCone()
{
        int num;
        int i, k;
        entity_t *mapent;
        epair_t *ep;

        num = g_bspdata->numtexrefs;
        g_lightingconeinfo = (vec3_t *)malloc( num * sizeof( vec3_t ) );
        for ( i = 0; i < num; i++ )
        {
                g_lightingconeinfo[i][0] = 1.0; // default power
                g_lightingconeinfo[i][1] = 1.0; // default scale
        }
        for ( k = 0; k < g_bspdata->numentities; k++ )
        {
                mapent = &g_bspdata->entities[k];
                if ( strcmp( ValueForKey( mapent, "classname" ), "info_angularfade" ) )
                        continue;
                Developer( DEVELOPER_LEVEL_MESSAGE, "info_angularfade entity detected.\n" );
                for ( i = 0; i < num; i++ )
                {
                        const char *texname = ( &g_bspdata->dtexrefs[num] )->name;
                        for ( ep = mapent->epairs; ep; ep = ep->next )
                        {
                                if ( strcasecmp( ep->key, texname ) )
                                        continue;
                                if ( !strcasecmp( ep->key, "origin" ) )
                                        continue;
                                double power, scale;
                                int count;
                                count = sscanf( ep->value, "%lf %lf", &power, &scale );
                                if ( count == 1 )
                                {
                                        scale = 1.0;
                                }
                                else if ( count != 2 )
                                {
                                        Warning( "ignore bad angular fade value '%s'", ep->value );
                                        continue;
                                }
                                if ( power < 0.0 || scale < 0.0 )
                                {
                                        Warning( "ignore disallowed angular fade value '%s'", ep->value );
                                        continue;
                                }
                                scale *= DefaultScaleForPower( power );
                                g_lightingconeinfo[i][0] = power;
                                g_lightingconeinfo[i][1] = scale;
                                Developer( DEVELOPER_LEVEL_MESSAGE, "info_angularfade: %s = %f %f\n", texname, power, scale );
                        }
                }
        }
}

// =====================================================================================
//  MakePatches
// =====================================================================================
static entity_t *FindTexlightEntity( int facenum )
{
        dface_t *face = &g_bspdata->dfaces[facenum];
        const dplane_t *dplane = getPlaneFromFace( face );
        const char *texname = GetTextureByNumber( g_bspdata, face->texinfo );
        entity_t *faceent = g_face_entity[facenum];
        vec3_t centroid;
        Winding *w = new Winding( *face, g_bspdata );
        w->getCenter( centroid );
        delete w;
        VectorAdd( centroid, g_face_offset[facenum], centroid );

        entity_t *found = NULL;
        vec_t bestdist = -1;
        for ( int i = 0; i < g_bspdata->numentities; i++ )
        {
                entity_t *ent = &g_bspdata->entities[i];
                if ( strcmp( ValueForKey( ent, "classname" ), "light_surface" ) )
                        continue;
                if ( strcasecmp( ValueForKey( ent, "_tex" ), texname ) )
                        continue;
                vec3_t delta;
                GetVectorDForKey( ent, "origin", delta );
                VectorSubtract( delta, centroid, delta );
                vec_t dist = VectorLength( delta );
                if ( *ValueForKey( ent, "_frange" ) )
                {
                        if ( dist > FloatForKey( ent, "_frange" ) )
                                continue;
                }
                if ( *ValueForKey( ent, "_fdist" ) )
                {
                        if ( fabs( DotProduct( delta, dplane->normal ) ) > FloatForKey( ent, "_fdist" ) )
                                continue;
                }
                if ( *ValueForKey( ent, "_fclass" ) )
                {
                        if ( strcmp( ValueForKey( faceent, "classname" ), ValueForKey( ent, "_fclass" ) ) )
                                continue;
                }
                if ( *ValueForKey( ent, "_fname" ) )
                {
                        if ( strcmp( ValueForKey( faceent, "targetname" ), ValueForKey( ent, "_fname" ) ) )
                                continue;
                }
                if ( bestdist >= 0 && dist > bestdist )
                        continue;
                found = ent;
                bestdist = dist;
        }
        return found;
}

static int num_degenerate_faces = 0;
static int fakeplanes = 0;

static void MakePatchForFace( int facenum, Winding *w )
{
        dface_t *f = g_bspdata->dfaces + facenum;
        float area;
        LVector3 centroid( 0 );
        int i, j;
        texinfo_t *tex;

        tex = g_bspdata->texinfo + f->texinfo;

        area = w->getArea();
        if ( area <= 0 )
        {
                num_degenerate_faces++;
                return;
        }

        totalarea += area;

        // get a patch
        size_t patchidx = g_patches.size();
        patch_t patch;
        memset( &patch, 0, sizeof( patch_t ) );
        patch.next = -1;
        patch.nextparent = -1;
        patch.nextclusterchild = -1;
        patch.child1 = -1;
        patch.child2 = -1;
        patch.parent = -1;
        patch.bumped = (byte)( g_textures[tex->texref].bump != nullptr );

        // link and save patch data
        patch.next = g_face_patches[facenum];
        g_face_patches[facenum] = patchidx;

        // compute a separate chop sacle for chop - since the patch "scale" is the texture scale
        // we want textures with higher resolution lightmap to be chopped up more
        float chopscale[2];
        chopscale[0] = chopscale[1] = 16.0f;
        if ( g_texscale )
        {
                // compute the texture "scale" in s, t
                for ( i = 0; i < 2; i++ )
                {
                        patch.scale[i] = 0.0f;
                        chopscale[i] = 0.0f;
                        for ( j = 0; j < 3; j++ )
                        {
                                patch.scale[i] += tex->vecs[i][j] *
                                        tex->vecs[i][j];
                                chopscale[i] += tex->lightmap_vecs[i][j] *
                                        tex->lightmap_vecs[i][j];
                        }
                        patch.scale[i] = std::sqrt( patch.scale[i] );
                        chopscale[i] = std::sqrt( chopscale[i] );
                }
        }
        else
        {
                patch.scale[0] = patch.scale[1] = 1.0f;
        }

        patch.area = area;

        patch.sky = GetTextureContents( g_bspdata->dtexrefs[tex->texref].name ) == CONTENTS_SKY;

        // chop scaled up lightmaps coarser
        patch.luxscale = ( chopscale[0] + chopscale[1] ) / 2;
        patch.chop = g_maxchop;

        patch.winding = w;

        patch.plane = (dplane_t *)getPlaneFromFace( f );

        if ( g_face_offset[facenum][0] || g_face_offset[facenum][1] || g_face_offset[facenum][2] )
        {
                dplane_t *pl;

                if ( g_bspdata->numplanes + fakeplanes >= MAX_MAP_PLANES )
                {
                        Error( "numplanes + fakeplanes >= MAX_MAP_PLANES" );
                }

                pl = g_bspdata->dplanes + ( g_bspdata->numplanes + fakeplanes );
                fakeplanes++;

                *pl = *( patch.plane );
                pl->dist += DotProduct( g_face_offset[facenum], pl->normal );
                patch.plane = pl;
        }

        patch.facenum = facenum;

        vec3_t pcenter;
        w->getCenter( pcenter );
        VectorCopy( pcenter, patch.origin );

        // save "Center" for generating the face normals later
        VectorSubtract( patch.origin, g_face_offset[facenum], g_face_centroids[facenum] );

        VectorCopy( patch.plane->normal, patch.normal );

        vec3_t vmins, vmaxs;
        w->getBounds( vmins, vmaxs );
        VectorCopy( vmins, patch.face_mins );
        VectorCopy( vmaxs, patch.face_maxs );
        VectorCopy( patch.face_mins, patch.mins );
        VectorCopy( patch.face_maxs, patch.maxs );

        BaseLightForFace( f, patch.baselight, &patch.basearea, patch.reflectivity );

        // chop all texlights very fine
        //if ( !VectorCompare( patch.baselight, vec3_origin ) )
        //{
        //
        //}

        // get rid of extra functionality on displacement surfaces
        // nope

        g_patches.push_back( patch );
}

static void MakePatches()
{
        int     i, j;
        dface_t *f;
        int fn;
        Winding *w;
        dmodel_t *mod;
        LVector3 origin;
        entity_t *ent;

        printf( "%i faces\n", g_bspdata->numfaces );

        for ( i = 0; i < g_bspdata->nummodels; i++ )
        {
                mod = g_bspdata->dmodels + i;
                ent = EntityForModel( g_bspdata, i );
                VectorCopy( vec3_origin, origin );

                // bmodels with origin brushes need to be offset into
                // their in-use position
                vec3_t vorigin;
                GetVectorDForKey( ent, "origin", vorigin );
                VectorCopy( vorigin, origin );
                for ( j = 0; j < mod->numfaces; j++ )
                {
                        fn = mod->firstface + j;
                        g_face_entity[fn] = ent;
                        VectorCopy( origin, g_face_offset[fn] );
                        f = g_bspdata->dfaces + fn;
                        w = WindingFromFace( f );
                        MakePatchForFace( fn, w );
                }
        }

        if ( num_degenerate_faces > 0 )
        {
                printf( "%d degenerate faces\n", num_degenerate_faces );
        }

        printf( "%i square feet [%.2f square inches]\n", (int)( totalarea / 144 ), totalarea );
}

bool PreventSubdivision( patch_t *patch )
{
        if ( GetTextureContents( g_bspdata->dtexrefs[g_bspdata->texinfo[g_bspdata->dfaces[patch->facenum].texinfo].texref].name ) == CONTENTS_SKY )
        {
                return true;
        }

        return false;
}

int CreateChildPatch( int parentnum, Winding *w, float area, const LVector3 &center )
{
        int childidx = (int)g_patches.size();

        patch_t child;
        patch_t *parent = &g_patches[parentnum];

        // copy all elements of parent patch to children
        child = *parent;

        // set up links
        child.next = -1;
        child.nextparent = -1;
        child.nextclusterchild = -1;
        child.child1 = -1;
        child.child2 = -1;
        child.parent = parentnum;
        child.iteration_key = 0;

        child.winding = w;
        child.area = area;

        VectorCopy( center, child.origin );
        GetPhongNormal( child.facenum, child.origin, child.normal );

        child.plane_dist = child.plane->dist;

        vec3_t vmins, vmaxs;
        child.winding->getBounds( vmins, vmaxs );
        VectorCopy( vmins, child.mins );
        VectorCopy( vmaxs, child.maxs );

        if ( child.baselight.length() == 0 )
        {
                g_patches.push_back( child );
                return childidx;
        }
                

        // subdivide patch towards minchop if on the edge of the face
        LVector3 total;
        VectorSubtract( child.maxs, child.mins, total );
        VectorScale( total, child.luxscale, total );

        if ( child.chop > g_minchop && ( total[0] < child.chop ) && ( total[1] < child.chop ) && ( total[2] < child.chop ) )
        {
                for ( int i = 0; i < 3; ++i )
                {
                        if ( ( child.face_maxs[i] == child.maxs[i] || child.face_mins[i] == child.mins[i] )
                             && total[i] > g_minchop )
                        {
                                child.chop = std::max( g_minchop, child.chop / 2 );
                                break;
                        }
                }
        }

        ThreadLock();
        g_patches.push_back( child );
        ThreadUnlock();

        return childidx;
}

void SubdividePatch( int patchnum )
{
        Winding *w, *o1, *o2;
        LVector3 total;
        LVector3 split;
        float dist;
        float widest = -1;
        int i, widest_axis = -1;
        bool subdivide = false;

        o1 = nullptr;
        o2 = nullptr;

        // get the current patch
        patch_t *patch = &g_patches[patchnum];
        if ( !patch )
        {
                return;
        }   

        // never subdivide sky patches
        if ( patch->sky )
        {
                return;
        }

        // get the patch winding
        w = patch->winding;

        // subdivide along the widest axis
        VectorSubtract( patch->maxs, patch->mins, total );
        VectorScale( total, patch->luxscale, total );
        for ( i = 0; i < 3; i++ )
        {
                if ( total[i] > widest )
                {
                        widest_axis = i;
                        widest = total[i];
                }

                if ( ( total[i] >= patch->chop ) && total[i] >= g_minchop )
                {
                        subdivide = true;
                }
        }

        if ( !subdivide && widest_axis != -1 )
        {
                // make more square
                if ( total[widest_axis] > total[( widest_axis + 1 ) % 3] * 2 && total[widest_axis] > total[( widest_axis + 2 ) % 3] * 2 )
                {
                        if ( patch->chop > g_minchop )
                        {
                                subdivide = true;
                                patch->chop = std::max( g_minchop, patch->chop / 2 );
                        }
                }
        }

        if ( !subdivide )
        {
                return;
        } 
                

        // split the winding
        VectorCopy( vec3_origin, split );
        split[widest_axis] = 1;
        dist = ( patch->mins[widest_axis] + patch->maxs[widest_axis] ) * 0.5f;
        vec3_t vsplit;
        VectorCopy( split, vsplit );
        w->Clip( vsplit, dist, &o1, &o2, ON_EPSILON );

        // calculate the area of the patches to see if they are "significant"
        vec3_t center1, center2;
        float area1 = 0, area2 = 0;
        if ( o1 != nullptr )
                area1 = o1->getAreaAndBalancePoint( center1 );
        if ( o2 != nullptr )
                area2 = o2->getAreaAndBalancePoint( center2 );

        if ( area1 == 0 || area2 == 0 )
        {
                Log( "zero area child patch\n" );
                return;
        }

        // create new child patches
        int ndxchild1 = CreateChildPatch( patchnum, o1, area1, GetLVector3( center1 ) );
        int ndxchild2 = CreateChildPatch( patchnum, o2, area2, GetLVector3( center2 ) );

        patch = &g_patches[patchnum];
        patch->child1 = ndxchild1;
        patch->child2 = ndxchild2;

        SubdividePatch( ndxchild1 );
        SubdividePatch( ndxchild2 );
}

static void SubdividePatches()
{
        if ( g_numbounce == 0 )
                return;

        size_t i, num;
        size_t patch_count = g_patches.size();
        printf( "%i patches before subdivision\n", patch_count );

        for ( i = 0; i < patch_count; i++ )
        {
                patch_t *cur = &g_patches[i];
                cur->plane_dist = cur->plane->dist;

                cur->nextparent = g_face_parents[cur->facenum];
                g_face_parents[cur->facenum] = cur - g_patches.data();
        }

        for ( i = 0; i < patch_count; i++ )
        {
                patch_t *patch = &g_patches[i];
                patch->parent = -1;
                if ( PreventSubdivision( patch ) )
                        continue;

                if ( !g_fastmode )
                {
                        SubdividePatch( i );
                }
        }

        // fixup next pointers
        for ( i = 0; i < (size_t)g_bspdata->numfaces; i++ )
        {
                g_face_patches[i] = -1;
        }

        patch_count = g_patches.size();
        for ( i = 0; i < patch_count; i++ )
        {
                patch_t *cur = &g_patches[i];
                cur->next = g_face_patches[cur->facenum];
                g_face_patches[cur->facenum] = cur - g_patches.data();
        }

        // Cache off the leaf number:
        // We have to do this after subdivision because some patches span leaves.
        // (only the faces for model #0 are split by it's BSP which is what governs the PVS, and the leaves we're interested in)
        // Sub models (1-255) are only split for the BSP that their model forms.
        // When those patches are subdivided their origins can end up in a different leaf.
        // The engine will split (clip) those faces at run time to the world BSP because the models
        // are dynamic and can be moved.  In the software renderer, they must be split exactly in order
        // to sort per polygon.
        for ( i = 0; i < patch_count; i++ )
        {
                float vorg[3];
                VectorCopy( g_patches[i].origin, vorg );
                g_patches[i].leafnum = PointInLeaf( vorg ) - g_bspdata->dleafs;

                //
                // test for point in solid space (can happen with detail and displacement surfaces)
                //
                if ( g_patches[i].leafnum == -1 )
                {
                        for ( int j = 0; j < g_patches[i].winding->m_NumPoints; j++ )
                        {
                                int clusterNumber = PointInLeafD( g_patches[i].winding->m_Points[j] ) - g_bspdata->dleafs;
                                if ( clusterNumber != -1 )
                                {
                                        g_patches[i].leafnum = clusterNumber;
                                        break;
                                }
                        }
                }
        }

        // build the list of patches that need to be lit
        for ( num = 0; num < patch_count; num++ )
        {
                // do them in reverse order
                i = patch_count - num - 1;

                // skip patches with children
                patch_t *pCur = &g_patches[i];
                if ( pCur->child1 == -1 )
                {
                        if ( pCur->leafnum != -1 )
                        {
                                pCur->nextclusterchild = g_cluster_children[pCur->leafnum];
                                g_cluster_children[pCur->leafnum] = pCur - g_patches.data();
                        }
                }
        }

        printf( "%i patches after subdivision\n", patch_count );
}

//=====================================================================

// =====================================================================================
//  CollectLight
// patch's totallight += new light received to each patch
// patch's emitlight = addlight (newly received light from GatherLight)
// patch's addlight = 0
// pull received light from children.
// =====================================================================================
static void     CollectLight( LVector3 &total )
{
        int i, j;
        patch_t *patch;

        total.set( 0.0, 0.0, 0.0 );

        // process patches in reverse order so that children are processed before their parents
        size_t patch_count = g_patches.size();
        for ( i = patch_count - 1; i >= 0; i-- )
        {
                patch = &g_patches[i];
                int normal_count = patch->bumped ? NUM_BUMP_VECTS + 1 : 1;

                if ( patch->sky )
                {
                        VectorFill( emitlight[i], 0 );
                }
                else if ( patch->child1 == -1 )
                {
                        // leaf node in patch tree
                        for ( j = 0; j < normal_count; j++ )
                        {
                                VectorAdd( patch->totallight.light[j], addlight[i].light[j], patch->totallight.light[j] );
                        }
                        VectorCopy( addlight[i].light[0], emitlight[i] );
                        VectorAdd( total, emitlight[i], total );
                }
                else
                {
                        // this is an interior node.
                        // pull received ilght from children

                        float s1, s2;
                        patch_t *child1, *child2;

                        child1 = &g_patches[patch->child1];
                        child2 = &g_patches[patch->child2];

                        if ( (int)patch->area != (int)( child1->area + child2->area ) )
                                s1 = 0;

                        s1 = child1->area / ( child1->area + child2->area );
                        s2 = child2->area / ( child1->area + child2->area );

                        // patch->totallight = s1 * child1->totallight + s2 * child2->totallight
                        for ( j = 0; j < normal_count; j++ )
                        {
                                VectorScale( child1->totallight.light[j], s1, patch->totallight.light[j] );
                                VectorMA( patch->totallight.light[j], s2, child2->totallight.light[j], patch->totallight.light[j] );
                        }

                        // patch->emitlight = s1 * child1->emitlight + s2 * child2->emitlight
                        VectorScale( emitlight[patch->child1], s1, emitlight[i] );
                }

                for ( j = 0; j < NUM_BUMP_VECTS + 1; j++ )
                {
                        VectorFill( addlight[i].light[j], 0 );
                }
        }
}

// =====================================================================================
//  GatherLight
//      Get light from other g_patches
//      Run multi-threaded
// =====================================================================================
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 4100)                             // unreferenced formal parameter
#endif

void GatherLight( int threadnum )
{
        int i, j, k;
        transfer_t *trans;
        int num;
        patch_t *patch;
        LVector3 sum, v;

        while ( 1 )
        {
                j = GetThreadWork();
                if ( j == -1 )
                        break;

                patch = &g_patches[j];

                trans = patch->transfers;
                if ( !trans )
                        continue;

                num = patch->numtransfers;
                if ( patch->bumped )
                {
                        LVector3 delta;
                        LVector3 bumpsum[NUM_BUMP_VECTS + 1];
                        LVector3 normals[NUM_BUMP_VECTS + 1];

                        GetPhongNormal( patch->facenum, patch->origin, normals[0] );

                        texinfo_t *tinfo = &g_bspdata->texinfo[g_bspdata->dfaces[patch->facenum].texinfo];
                        // use facenormal along with the smooth normal to build the three bump map vectors
                        GetBumpNormals( GetLVector3_2( tinfo->vecs[0] ),
                                        GetLVector3_2( tinfo->vecs[1] ), patch->normal,
                                        normals[0], &normals[1] );

                        // force the base lightmap to use the flat normal instead of the phong normal
                        // FIXME: why does the patch not use the phong normal?
                        normals[0] = patch->normal;

                        for ( i = 0; i < NUM_BUMP_VECTS + 1; i++ )
                        {
                                VectorFill( bumpsum[i], 0 );
                        }

                        float dot;
                        for ( k = 0; k < num; k++, trans++ )
                        {
                                patch_t *patch2 = &g_patches[trans->patch];

                                // get vector to other patch
                                VectorSubtract( patch2->origin, patch->origin, delta );
                                delta.normalize();
                                // find light emitted from other patch
                                for ( i = 0; i < 3; i++ )
                                {
                                        v[i] = emitlight[trans->patch][i] * patch2->reflectivity[i];
                                }
                                // remove normal already factored into transfer steradian
                                float scale = 1.0f / DotProduct( delta, patch->normal );
                                VectorScale( v, trans->transfer * scale, v );

                                LVector3 bumpTransfer;
                                for ( i = 0; i < NUM_BUMP_VECTS + 1; i++ )
                                {
                                        dot = DotProduct( delta, normals[i] );
                                        if ( dot <= 0 )
                                        {
//						Assert( i > 0 ); // if this hits, then the transfer shouldn't be here.  It doesn't face the flat normal of this face!
                                                continue;
                                        }
                                        bumpTransfer = v * dot;
                                        VectorAdd( bumpsum[i], bumpTransfer, bumpsum[i] );
                                }
                        }

                        for ( i = 0; i < NUM_BUMP_VECTS + 1; i++ )
                        {
                                VectorCopy( bumpsum[i], addlight[j].light[i] );
                        }
                }
                else
                {
                        VectorFill( sum, 0 );
                        for ( k = 0; k < num; k++, trans++ )
                        {
                                for ( i = 0; i < 3; i++ )
                                {
                                        v[i] = emitlight[trans->patch][i] * g_patches[trans->patch].reflectivity[i];
                                }
                                VectorScale( v, trans->transfer, v );
                                VectorAdd( sum, v, sum );
                        }
                        VectorCopy( sum, addlight[j].light[0] );
                }
        }
}

#ifdef _WIN32
#pragma warning(pop)
#endif

// =====================================================================================
//  BounceLight
// =====================================================================================
static void     BounceLight()
{
        unsigned        i;
        char            name[64];

        bool keep_bouncing = g_numbounce > 0;

        for ( i = 0; i < g_patches.size(); i++ )
        {
                patch_t *patch = &g_patches[i];
                // totallight has a copy of the direct lighting.  Move it to the emitted light and zero it out (to integrate bounces only)
                VectorCopy( patch->totallight.light[0], emitlight[i] );
                // NOTE: This means that only the bounced light is integrated into totallight!
                VectorFill( patch->totallight.light[0], 0 );
        }

        LVector3 last_added( 0 );
        i = 0;
        while ( keep_bouncing )
        {
                // transfer light from to the leaf patches from other patches via transfers
                // this moves shooter->emitlight to receiver->addlight
                NamedRunThreadsOn( g_patches.size(), g_estimate, GatherLight );

                // move newly received light (addlight) to light to be sent out (emitlight)
                // start at children and pull light up to parents
                // light is always received to leaf patches
                LVector3 added( 0 );
                CollectLight( added );

                printf( "\tBounce #%i added RGB(%.0f, %.0f, %.0f)\n", i + 1, added[0], added[1], added[2] );

                if ( i + 1 == g_numbounce || ( added[0] < 1.0 && added[1] < 1.0 && added[2] < 1.0 ) )
                        keep_bouncing = false;

                i++;
        }
}

float FormFactorPolyToDiff( patch_t *polygon, patch_t *diff )
{
        Winding *w = polygon->winding;

        float formfac = 0.0;

        for ( int i = 0; i < w->m_NumPoints; i++ )
        {
                int inext = ( i < w->m_NumPoints - 1 ) ? i + 1 : 0;

                vec3_t vGammaVector, vVector1, vVector2;
                VectorSubtract( w->m_Points[i], diff->origin, vVector1 );
                VectorSubtract( w->m_Points[inext], diff->origin, vVector2 );
                VectorNormalize( vVector1 );
                VectorNormalize( vVector2 );
                CrossProduct( vVector1, vVector2, vGammaVector );
                float flSinAlpha = VectorNormalize( vGammaVector );
                if ( flSinAlpha < -1.0f || flSinAlpha > 1.0f )
                        return 0.0f;
                float sc = std::asin( flSinAlpha );
                VectorScale( vGammaVector, sc, vGammaVector );

                formfac += DotProduct( vGammaVector, diff->normal );
        }

        formfac *= ( 0.5 / polygon->area );

        return formfac;
}

float FormFactorDiffToDiff( patch_t *diff1, patch_t *diff2 )
{
        LVector3 vdelta = diff1->origin - diff2->origin;
        float len = vdelta.length();
        vdelta.normalize();

        float scale = -vdelta.dot(diff1->normal) * vdelta.dot(diff2->normal) / ( len * len );

        return scale;
}

void MakeTransfer( int patchidx1, int patchidx2, transfer_t *all_transfers )
{
        LVector3 delta;
        vec_t scale;
        float trans;
        transfer_t *transfer;

        //
        // get patches
        //
        if ( patchidx1 == -1 || patchidx2 == -1 )
                return;

        patch_t *patch1 = &g_patches[patchidx1];
        patch_t *patch2 = &g_patches[patchidx2];

        // todo IsSky: return

        // overflow check!
        if ( patch1->numtransfers >= MAX_PATCHES )
        {
                return;
        }
                

        // hack for patch areas <= 0 (degenerate)
        if ( patch2->area <= 0 )
        {
                return;
        }
                

        transfer = &all_transfers[patch1->numtransfers];

        scale = FormFactorDiffToDiff( patch2, patch1 );

        if ( scale <= 0 )
        {
                return;
        }
                

        // test 5 times rule
        LVector3 vdelta = patch1->origin - patch2->origin;
        float thres = ( Q_PI * 0.04 ) * vdelta.dot( vdelta );

        if ( thres < patch2->area )
        {
                scale = FormFactorPolyToDiff( patch2, patch1 );
                if ( scale <= 0.0 )
                {
                        return;
                }
                        
        }

        trans = patch2->area * scale;
        if ( trans <= TRANSFER_EPSILON )
        {
                return;
        }
                

        transfer->patch = patch2 - g_patches.data();
        transfer->transfer = trans;

        patch1->numtransfers++;
}

static int max_transfer = 0;

void MakeScales( int patchidx, transfer_t *all_transfers )
{
        int j;
        float total;
        transfer_t *t, *t2;
        total = 0;

        if ( patchidx == -1 )
                return;

        patch_t *patch = &g_patches[patchidx];

        // copy the transfers out
        if ( patch->numtransfers )
        {
                if ( patch->numtransfers > max_transfer )
                        max_transfer = patch->numtransfers;

                patch->transfers = (transfer_t *)calloc( 1, patch->numtransfers * sizeof( transfer_t ) );
                if ( !patch->transfers )
                        Error( "Memory allocation failure" );

                // get total transfer energy
                t2 = all_transfers;

                // overflow check!
                for ( j = 0; j < patch->numtransfers; j++, t2++ )
                {
                        total += t2->transfer;
                }

                // the total transfer should be PI, but we need to correct errors due to overlapping surfaces
                if ( total < Q_PI )
                        total = 1.0 / total;
                else
                        total = 1.0 / Q_PI;

                t = patch->transfers;
                t2 = all_transfers;
                for ( j = 0; j < patch->numtransfers; j++, t++, t2++ )
                {
                        t->transfer = t2->transfer * total;
                        t->patch = t2->patch;
                }

                if ( patch->numtransfers > max_transfer )
                        max_transfer = patch->numtransfers;
        }

        ThreadLock();
        g_total_transfer += patch->numtransfers;
        ThreadUnlock();
}

void MakeAllScales()
{
        // determine visiblity between patches
        BuildVisMatrix();

        FreeVisMatrix();

        Log( "transfers %d, max %d\n", g_total_transfer, max_transfer );

        printf( "transfer lists: %5.1f megs\n",
                (float)g_total_transfer * sizeof( transfer_t ) / ( 1024 * 1024 ) );
}

static void     BuildRayTraceEnvironment()
{
        std::cout << "Building raytrace environment" << std::endl;

        RADTrace::scene->set_build_quality( RayTraceScene::BUILD_QUALITY_HIGH );

        // Group all world faces by their contents, make a separate
        // triangle mesh for each contents type

        SimpleHashMap<contents_t, pvector<dface_t *>, int_hash> contents2faces;

        for ( int mdlnum = 0; mdlnum < g_bspdata->nummodels; mdlnum++ )
        {
                // if not the world
                if ( mdlnum != 0 )
                {
                        entity_t *ent = EntityForModel( g_bspdata, mdlnum );
                        int lightflags = IntForKey( ent, "zhlt_lightflags" );
                        if ( lightflags == 0 )
                        {
                                // model is not opaque (doesn't block light)
                                // so we shouldn't create a ray trace mesh for it
                                continue;
                        }
                }

                // Setup ray tracing scene
                dmodel_t *mdl = &g_bspdata->dmodels[mdlnum];

                for ( int facenum = 0; facenum < mdl->numfaces; facenum++ )
                {
                        dface_t *face = &g_bspdata->dfaces[mdl->firstface + facenum];
                        texinfo_t *tex = &g_bspdata->texinfo[face->texinfo];
                        texref_t *tref = &g_bspdata->dtexrefs[tex->texref];
                        contents_t contents = GetTextureContents( tref->name );

                        PT( RayTraceTriangleMesh ) geom = new RayTraceTriangleMesh;
                        geom->set_mask( contents );
                        geom->set_build_quality( RayTraceScene::BUILD_QUALITY_HIGH );

                        int ntris = face->numedges - 2;
                        for ( int tri = 0; tri < ntris; tri++ )
                        {
                                geom->add_triangle( VertCoord( g_bspdata, face, 0 ),
                                        VertCoord( g_bspdata, face, ( tri + 1 ) % face->numedges ),
                                        VertCoord( g_bspdata, face, ( tri + 2 ) % face->numedges ) );
                        }

                        geom->build();

                        RADTrace::scene->add_geometry( geom );
                        g_rtroot.attach_new_node( geom );

                        RADTrace::dface_lookup[geom->get_geom_id()] = face;
                }
        }

        RADTrace::scene->update();
}

// =====================================================================================
//  RadWorld
// =====================================================================================
static void     RadWorld()
{
        PStatTimer _timer( radworld_collector );

        unsigned        i;
        unsigned        j;

        // setup our OpenCL environment for the GPU
        //CLHelper::SetupCL();

        // figure out how much memory all the lightmaps for this level
        // will take up on disk
        DetermineLightmapMemory();

        MakeBackplanes();
        MakeParents( 0, -1 );

        BuildRayTraceEnvironment();

        // TODO?
        //BuildClusterTable();

        // turn each face into a single patch
        MakePatches(); // done
        PairEdges(); // done

        // store the vertex normals calculated in PairEdges
        // so that the can be written to the bsp file for 
        // use in the engine
        SaveVertexNormals();

        SubdividePatches(); // done

        // create directlights out of patches and lights
        Lights::CreateDirectLights(); // done

        ScaleDirectLights();

        Log( "\n" );

        // go!

        // generate a position map for each face
        //NamedRunThreadsOnIndividual( g_bspdata->numfaces, g_estimate, FindFacePositions );

        bfl_collector.start();
        // build initial facelights
        lightinfo = (lightinfo_t *)malloc( g_bspdata->numfaces * sizeof( lightinfo_t ) );
        memset( lightinfo, 0, sizeof( lightinfo ) );
        NamedRunThreadsOnIndividual( g_bspdata->numfaces, g_estimate, BuildFacelights ); // done
        bfl_collector.stop();

        if ( g_numbounce > 0 )
        {
                // allocate memory for emitlight/addlight

                // build transfer lists
                //MakeScalesStub();

                emitlight.resize( g_patches.size() );
                memset( emitlight.data(), 0, g_patches.size() * sizeof( LVector3 ) );
                addlight.resize( g_patches.size() );
                memset( addlight.data(), 0, g_patches.size() * sizeof( bumpsample_t ) );

                MakeAllScales();

                // spread light around
                BounceLight();
        }

        //FreeTransfers();
        //FreeStyleArrays();

        //NamedRunThreadsOnIndividual( g_bspdata->numfaces, g_estimate, CreateTriangulations );

        // blend bounced light into direct light and save
        PrecompLightmapOffsets();

        NamedRunThreadsOnIndividual( g_bspdata->numfaces, g_estimate, FinalLightFace );
        if ( g_maxdiscardedlight > 0.01 )
        {
                Verbose( "Maximum brightness loss (too many light styles on a face) = %f @(%f, %f, %f)\n", g_maxdiscardedlight, g_maxdiscardedpos[0], g_maxdiscardedpos[1], g_maxdiscardedpos[2] );
        }

        // misc light computations
        LeafAmbientLighting::compute_per_leaf_ambient_lighting();
        DoComputeStaticPropLighting();

        // free up the direct lights now that we have facelights
        Lights::DeleteDirectLights();

        ReportRadTimers();
}

// =====================================================================================
//  Usage
// =====================================================================================
static void     Usage()
{
        Banner();

        Log( "\n-= %s Options =-\n\n", g_Program );
        Log( "    -lang file      : localization file\n" );
        Log( "    -waddir folder  : Search this folder for wad files.\n" );
        Log( "    -fast           : Fast rad\n" );
        Log( "    -vismatrix value: Set vismatrix method to normal, sparse or off .\n" );
        Log( "    -extra          : Improve lighting quality by doing 9 point oversampling\n" );
        Log( "    -bounce #       : Set number of radiosity bounces\n" );
        Log( "    -ambient r g b  : Set ambient world light (0.0 to 1.0, r g b)\n" );
        Log( "    -limiter #      : Set light clipping threshold (-1=None)\n" );
        Log( "    -circus         : Enable 'circus' mode for locating unlit lightmaps\n" );
        Log( "    -nospread       : Disable sunlight spread angles for this compile\n" );
        Log( "    -smooth #       : Set smoothing threshold for blending (in degrees)\n" );
        Log( "    -smooth2 #      : Set smoothing threshold between different textures\n" );
        Log( "    -chop #         : Set radiosity patch size for normal textures\n" );
        Log( "    -texchop #      : Set radiosity patch size for texture light faces\n\n" );
        Log( "    -notexscale     : Do not scale radiosity patches with texture scale\n" );
        Log( "    -coring #       : Set lighting threshold before blackness\n" );
        Log( "    -dlight #       : Set direct lighting threshold\n" );
        Log( "    -nolerp         : Disable radiosity interpolation, nearest point instead\n\n" );
        Log( "    -fade #         : Set global fade (larger values = shorter lights)\n" );
        Log( "    -texlightgap #  : Set global gap distance for texlights\n" );
        Log( "    -scale #        : Set global light scaling value\n" );
        Log( "    -gamma #        : Set global gamma value\n\n" );
        Log( "    -sky #          : Set ambient sunlight contribution in the shade outside\n" );
        Log( "    -lights file    : Manually specify a lights.rad file to use\n" );
        Log( "    -noskyfix       : Disable light_environment being global\n" );
        Log( "    -incremental    : Use or create an incremental transfer list file\n\n" );
        Log( "    -dump           : Dumps light patches to a file for hlrad debugging info\n\n" );
        Log( "    -texdata #      : Alter maximum texture memory limit (in kb)\n" );
        Log( "    -lightdata #    : Alter maximum lighting memory limit (in kb)\n" ); //lightdata
        Log( "    -chart          : display bsp statitics\n" );
        Log( "    -low | -high    : run program an altered priority level\n" );
        Log( "    -nolog          : Do not generate the compile logfiles\n" );
        Log( "    -threads #      : manually specify the number of threads to run\n" );
#ifdef _WIN32
        Log( "    -estimate       : display estimated time during compile\n" );
#endif
#ifdef SYSTEM_POSIX
        Log( "    -noestimate     : Do not display continuous compile time estimates\n" );
#endif
        Log( "    -verbose        : compile with verbose messages\n" );
        Log( "    -noinfo         : Do not show tool configuration information\n" );
        Log( "    -dev #          : compile with developer message\n\n" );

        // ------------------------------------------------------------------------
        // Changes by Adam Foster - afoster@compsoc.man.ac.uk

        // AJM: we dont need this extra crap
        //Log("-= Unofficial features added by Adam Foster (afoster@compsoc.man.ac.uk) =-\n\n");
        Log( "   -colourgamma r g b  : Sets different gamma values for r, g, b\n" );
        Log( "   -colourscale r g b  : Sets different lightscale values for r, g ,b\n" );
        Log( "   -colourjitter r g b : Adds noise, independent colours, for dithering\n" );
        Log( "   -jitter r g b       : Adds noise, monochromatic, for dithering\n" );
        //Log("-= End of unofficial features! =-\n\n" );

        // ------------------------------------------------------------------------  

        Log( "   -customshadowwithbounce : Enables custom shadows with bounce light\n" );
        Log( "   -rgbtransfers           : Enables RGB Transfers (for custom shadows)\n\n" );

        Log( "   -minlight #    : Minimum final light (integer from 0 to 255)\n" );
        Log( "   -softsky #     : Smooth skylight.(0=off 1=on)\n" );
        Log( "   -depth #       : Thickness of translucent objects.\n" );
        Log( "   -notextures    : Don't load textures.\n" );
        Log( "   -texreflectgamma # : Gamma that relates reflectivity to texture color bits.\n" );
        Log( "   -texreflectscale # : Reflectivity for 255-white texture.\n" );
        Log( "   -blur #        : Enlarge lightmap sample to blur the lightmap.\n" );
        Log( "   -noemitterrange: Don't fix pointy texlights.\n" );
        Log( "   -nobleedfix    : Don't fix wall bleeding problem for large blur value.\n" );
        Log( "   -drawpatch     : Export light patch positions to file 'mapname_patch.pts'.\n" );
        Log( "   -drawsample x y z r    : Export light sample positions in an area to file 'mapname_sample.pts'.\n" );
        Log( "   -drawedge      : Export smooth edge positions to file 'mapname_edge.pts'.\n" );
        Log( "   -drawlerp      : Show bounce light triangulation status.\n" );
        Log( "   -drawnudge     : Show nudged samples.\n" );
        Log( "   -drawoverload  : Highlight fullbright spots\n" );

        Log( "    mapfile       : The mapfile to compile\n\n" );

        exit( 1 );
}

// =====================================================================================
//  Settings
// =====================================================================================
static void     Settings()
{
        char*           tmp;
        char            buf1[1024];
        char            buf2[1024];

        if ( !g_info )
        {
                return;
        }

        Log( "\n-= Current %s Settings =-\n", g_Program );
        Log( "Name                | Setting             | Default\n"
             "--------------------|---------------------|-------------------------\n" );

        // ZHLT Common Settings
        if ( DEFAULT_NUMTHREADS == -1 )
        {
                Log( "threads              [ %17d ] [            Varies ]\n", g_numthreads );
        }
        else
        {
                Log( "threads              [ %17d ] [ %17d ]\n", g_numthreads, DEFAULT_NUMTHREADS );
        }

        Log( "verbose              [ %17s ] [ %17s ]\n", g_verbose ? "on" : "off", DEFAULT_VERBOSE ? "on" : "off" );
        Log( "log                  [ %17s ] [ %17s ]\n", g_log ? "on" : "off", DEFAULT_LOG ? "on" : "off" );
        Log( "developer            [ %17d ] [ %17d ]\n", g_developer, DEFAULT_DEVELOPER );
        Log( "chart                [ %17s ] [ %17s ]\n", g_chart ? "on" : "off", DEFAULT_CHART ? "on" : "off" );
        Log( "estimate             [ %17s ] [ %17s ]\n", g_estimate ? "on" : "off", DEFAULT_ESTIMATE ? "on" : "off" );
        Log( "max texture memory   [ %17d ] [ %17d ]\n", g_max_map_texref, DEFAULT_MAX_MAP_TEXREF );
        Log( "max lighting memory  [ %17d ] [ %17d ]\n", g_max_map_lightdata, DEFAULT_MAX_MAP_LIGHTDATA ); //lightdata

        switch ( g_threadpriority )
        {
        case TP_normal:
        default:
                tmp = "Normal";
                break;
        case TP_low:
                tmp = "Low";
                break;
        case TP_high:
                tmp = "High";
                break;
        }
        Log( "priority             [ %17s ] [ %17s ]\n", tmp, "Normal" );
        Log( "\n" );

        Log( "fast rad             [ %17s ] [ %17s ]\n", g_fastmode ? "on" : "off", DEFAULT_FASTMODE ? "on" : "off" );
        Log( "vismatrix algorithm  [ %17s ] [ %17s ]\n",
             g_method == eMethodVismatrix ? "Original" : g_method == eMethodSparseVismatrix ? "Sparse" : g_method == eMethodNoVismatrix ? "NoMatrix" : "Unknown",
             DEFAULT_METHOD == eMethodVismatrix ? "Original" : DEFAULT_METHOD == eMethodSparseVismatrix ? "Sparse" : DEFAULT_METHOD == eMethodNoVismatrix ? "NoMatrix" : "Unknown"
        );
        Log( "oversampling (-extra)[ %17s ] [ %17s ]\n", g_extra ? "on" : "off", DEFAULT_EXTRA ? "on" : "off" );
        Log( "bounces              [ %17d ] [ %17d ]\n", g_numbounce, DEFAULT_BOUNCE );

        safe_snprintf( buf1, sizeof( buf1 ), "%1.3f %1.3f %1.3f", g_ambient[0], g_ambient[1], g_ambient[2] );
        safe_snprintf( buf2, sizeof( buf2 ), "%1.3f %1.3f %1.3f", DEFAULT_AMBIENT_RED, DEFAULT_AMBIENT_GREEN, DEFAULT_AMBIENT_BLUE );
        Log( "ambient light        [ %17s ] [ %17s ]\n", buf1, buf2 );
        safe_snprintf( buf1, sizeof( buf1 ), "%3.3f", g_limitthreshold );
        safe_snprintf( buf2, sizeof( buf2 ), "%3.3f", DEFAULT_LIMITTHRESHOLD );
        Log( "light limit threshold[ %17s ] [ %17s ]\n", g_limitthreshold >= 0 ? buf1 : "None", buf2 );
        Log( "circus mode          [ %17s ] [ %17s ]\n", g_circus ? "on" : "off", DEFAULT_CIRCUS ? "on" : "off" );

        Log( "\n" );

        safe_snprintf( buf1, sizeof( buf1 ), "%3.3f", g_smoothing_value );
        safe_snprintf( buf2, sizeof( buf2 ), "%3.3f", DEFAULT_SMOOTHING_VALUE );
        Log( "smoothing threshold  [ %17s ] [ %17s ]\n", buf1, buf2 );
        safe_snprintf( buf1, sizeof( buf1 ), g_smoothing_value_2<0 ? "no change" : "%3.3f", g_smoothing_value_2 );
        safe_snprintf( buf2, sizeof( buf2 ), DEFAULT_SMOOTHING2_VALUE<0 ? "no change" : "%3.3f", DEFAULT_SMOOTHING2_VALUE );
        Log( "smoothing threshold 2[ %17s ] [ %17s ]\n", buf1, buf2 );
        safe_snprintf( buf1, sizeof( buf1 ), "%3.3f", g_dlight_threshold );
        safe_snprintf( buf2, sizeof( buf2 ), "%3.3f", DEFAULT_DLIGHT_THRESHOLD );
        Log( "direct threshold     [ %17s ] [ %17s ]\n", buf1, buf2 );
        safe_snprintf( buf1, sizeof( buf1 ), "%3.3f", g_direct_scale );
        safe_snprintf( buf2, sizeof( buf2 ), "%3.3f", DEFAULT_DLIGHT_SCALE );
        Log( "direct light scale   [ %17s ] [ %17s ]\n", buf1, buf2 );
        safe_snprintf( buf1, sizeof( buf1 ), "%3.3f", g_coring );
        safe_snprintf( buf2, sizeof( buf2 ), "%3.3f", DEFAULT_CORING );
        Log( "coring threshold     [ %17s ] [ %17s ]\n", buf1, buf2 );
        //Log( "patch interpolation  [ %17s ] [ %17s ]\n", g_lerp_enabled ? "on" : "off", DEFAULT_LERP_ENABLED ? "on" : "off" );

        Log( "\n" );

        Log( "texscale             [ %17s ] [ %17s ]\n", g_texscale ? "on" : "off", DEFAULT_TEXSCALE ? "on" : "off" );
        Log( "patch subdividing    [ %17s ] [ %17s ]\n", g_subdivide ? "on" : "off", DEFAULT_SUBDIVIDE ? "on" : "off" );
        safe_snprintf( buf1, sizeof( buf1 ), "%3.3f", g_chop );
        safe_snprintf( buf2, sizeof( buf2 ), "%3.3f", DEFAULT_CHOP );
        Log( "chop value           [ %17s ] [ %17s ]\n", buf1, buf2 );
        safe_snprintf( buf1, sizeof( buf1 ), "%3.3f", g_texchop );
        safe_snprintf( buf2, sizeof( buf2 ), "%3.3f", DEFAULT_TEXCHOP );
        Log( "texchop value        [ %17s ] [ %17s ]\n", buf1, buf2 );
        Log( "\n" );

        safe_snprintf( buf1, sizeof( buf1 ), "%3.3f", g_fade );
        safe_snprintf( buf2, sizeof( buf2 ), "%3.3f", DEFAULT_FADE );
        Log( "global fade          [ %17s ] [ %17s ]\n", buf1, buf2 );
        safe_snprintf( buf1, sizeof( buf1 ), "%3.3f", g_texlightgap );
        safe_snprintf( buf2, sizeof( buf2 ), "%3.3f", DEFAULT_TEXLIGHTGAP );
        Log( "global texlight gap  [ %17s ] [ %17s ]\n", buf1, buf2 );

        // ------------------------------------------------------------------------
        // Changes by Adam Foster - afoster@compsoc.man.ac.uk
        // replaces the old stuff for displaying current values for gamma and lightscale
        safe_snprintf( buf1, sizeof( buf1 ), "%1.3f %1.3f %1.3f", g_colour_lightscale[0], g_colour_lightscale[1], g_colour_lightscale[2] );
        safe_snprintf( buf2, sizeof( buf2 ), "%1.3f %1.3f %1.3f", DEFAULT_COLOUR_LIGHTSCALE_RED, DEFAULT_COLOUR_LIGHTSCALE_GREEN, DEFAULT_COLOUR_LIGHTSCALE_BLUE );
        Log( "global light scale   [ %17s ] [ %17s ]\n", buf1, buf2 );

        safe_snprintf( buf1, sizeof( buf1 ), "%1.3f %1.3f %1.3f", g_colour_qgamma[0], g_colour_qgamma[1], g_colour_qgamma[2] );
        safe_snprintf( buf2, sizeof( buf2 ), "%1.3f %1.3f %1.3f", DEFAULT_COLOUR_GAMMA_RED, DEFAULT_COLOUR_GAMMA_GREEN, DEFAULT_COLOUR_GAMMA_BLUE );
        Log( "global gamma         [ %17s ] [ %17s ]\n", buf1, buf2 );
        // ------------------------------------------------------------------------

        safe_snprintf( buf1, sizeof( buf1 ), "%3.3f", g_lightscale );
        safe_snprintf( buf2, sizeof( buf2 ), "%3.3f", DEFAULT_LIGHTSCALE );
        Log( "global light scale   [ %17s ] [ %17s ]\n", buf1, buf2 );


        safe_snprintf( buf1, sizeof( buf1 ), "%3.3f", g_indirect_sun );
        safe_snprintf( buf2, sizeof( buf2 ), "%3.3f", DEFAULT_INDIRECT_SUN );
        Log( "global sky diffusion [ %17s ] [ %17s ]\n", buf1, buf2 );

        Log( "\n" );
        Log( "spread angles        [ %17s ] [ %17s ]\n", g_allow_spread ? "on" : "off", DEFAULT_ALLOW_SPREAD ? "on" : "off" );
        Log( "sky lighting fix     [ %17s ] [ %17s ]\n", g_sky_lighting_fix ? "on" : "off", DEFAULT_SKY_LIGHTING_FIX ? "on" : "off" );
        Log( "incremental          [ %17s ] [ %17s ]\n", g_incremental ? "on" : "off", DEFAULT_INCREMENTAL ? "on" : "off" );
        Log( "dump                 [ %17s ] [ %17s ]\n", g_dumppatches ? "on" : "off", DEFAULT_DUMPPATCHES ? "on" : "off" );

        // ------------------------------------------------------------------------
        // Changes by Adam Foster - afoster@compsoc.man.ac.uk
        // displays information on all the brand-new features :)

        Log( "\n" );
        safe_snprintf( buf1, sizeof( buf1 ), "%3.1f %3.1f %3.1f", g_colour_jitter_hack[0], g_colour_jitter_hack[1], g_colour_jitter_hack[2] );
        safe_snprintf( buf2, sizeof( buf2 ), "%3.1f %3.1f %3.1f", DEFAULT_COLOUR_JITTER_HACK_RED, DEFAULT_COLOUR_JITTER_HACK_GREEN, DEFAULT_COLOUR_JITTER_HACK_BLUE );
        Log( "colour jitter        [ %17s ] [ %17s ]\n", buf1, buf2 );
        safe_snprintf( buf1, sizeof( buf1 ), "%3.1f %3.1f %3.1f", g_jitter_hack[0], g_jitter_hack[1], g_jitter_hack[2] );
        safe_snprintf( buf2, sizeof( buf2 ), "%3.1f %3.1f %3.1f", DEFAULT_JITTER_HACK_RED, DEFAULT_JITTER_HACK_GREEN, DEFAULT_JITTER_HACK_BLUE );
        Log( "monochromatic jitter [ %17s ] [ %17s ]\n", buf1, buf2 );



        // ------------------------------------------------------------------------

        Log( "\n" );
        Log( "custom shadows with bounce light\n"
             "                     [ %17s ] [ %17s ]\n", g_customshadow_with_bouncelight ? "on" : "off", DEFAULT_CUSTOMSHADOW_WITH_BOUNCELIGHT ? "on" : "off" );
        Log( "rgb transfers        [ %17s ] [ %17s ]\n", g_rgb_transfers ? "on" : "off", DEFAULT_RGB_TRANSFERS ? "on" : "off" );

        Log( "minimum final light  [ %17d ] [ %17d ]\n", (int)g_minlight, (int)DEFAULT_MINLIGHT );
        Log( "size of transfer     [ %17s ] [ %17s ]\n", buf1, buf2 );
        Log( "size of rgbtransfer  [ %17s ] [ %17s ]\n", buf1, buf2 );
        Log( "soft sky             [ %17s ] [ %17s ]\n", g_softsky ? "on" : "off", DEFAULT_SOFTSKY ? "on" : "off" );
        safe_snprintf( buf1, sizeof( buf1 ), "%3.3f", g_translucentdepth );
        safe_snprintf( buf2, sizeof( buf2 ), "%3.3f", DEFAULT_TRANSLUCENTDEPTH );
        Log( "translucent depth    [ %17s ] [ %17s ]\n", buf1, buf2 );
        Log( "ignore textures      [ %17s ] [ %17s ]\n", g_notextures ? "on" : "off", DEFAULT_NOTEXTURES ? "on" : "off" );
        safe_snprintf( buf1, sizeof( buf1 ), "%3.3f", g_texreflectgamma );
        safe_snprintf( buf2, sizeof( buf2 ), "%3.3f", DEFAULT_TEXREFLECTGAMMA );
        Log( "reflectivity gamma   [ %17s ] [ %17s ]\n", buf1, buf2 );
        safe_snprintf( buf1, sizeof( buf1 ), "%3.3f", g_texreflectscale );
        safe_snprintf( buf2, sizeof( buf2 ), "%3.3f", DEFAULT_TEXREFLECTSCALE );
        Log( "reflectivity scale   [ %17s ] [ %17s ]\n", buf1, buf2 );
        safe_snprintf( buf1, sizeof( buf1 ), "%3.3f", g_blur );
        safe_snprintf( buf2, sizeof( buf2 ), "%3.3f", DEFAULT_BLUR );
        Log( "blur size            [ %17s ] [ %17s ]\n", buf1, buf2 );
        Log( "no emitter range     [ %17s ] [ %17s ]\n", g_noemitterrange ? "on" : "off", DEFAULT_NOEMITTERRANGE ? "on" : "off" );
        Log( "wall bleeding fix    [ %17s ] [ %17s ]\n", g_bleedfix ? "on" : "off", DEFAULT_BLEEDFIX ? "on" : "off" );

        Log( "\n\n" );
}

// AJM: added in
// =====================================================================================
//  ReadInfoTexlights
//      try and parse texlight info from the info_texlights entity 
// =====================================================================================
void            ReadInfoTexlights()
{
        int         k;
        int         values;
        int         numtexlights = 0;
        float       r, g, b, i;
        entity_t*   mapent;
        epair_t*    ep;
        texlight_t  texlight;

        for ( k = 0; k < g_bspdata->numentities; k++ )
        {
                mapent = &g_bspdata->entities[k];

                if ( strcmp( ValueForKey( mapent, "classname" ), "info_texlights" ) )
                        continue;

                Log( "Reading texlights from info_texlights map entity\n" );

                for ( ep = mapent->epairs; ep; ep = ep->next )
                {
                        if ( !strcmp( ep->key, "classname" )
                             || !strcmp( ep->key, "origin" )
                             )
                                continue; // we dont care about these keyvalues

                        values = sscanf( ep->value, "%f %f %f %f", &r, &g, &b, &i );

                        if ( values == 1 )
                        {
                                g = b = r;
                        }
                        else if ( values == 4 ) // use brightness value.
                        {
                                r *= i / 255.0;
                                g *= i / 255.0;
                                b *= i / 255.0;
                        }
                        else if ( values != 3 )
                        {
                                Warning( "ignoring bad texlight '%s' in info_texlights entity", ep->key );
                                continue;
                        }

                        texlight.name = ep->key;
                        texlight.value[0] = r;
                        texlight.value[1] = g;
                        texlight.value[2] = b;
                        texlight.filename = "info_texlights";
                        s_texlights.push_back( texlight );
                        numtexlights++;
                }

        }
}


const char* lights_rad = "lights.rad";
const char* ext_rad = ".rad";
// =====================================================================================
//  LoadRadFiles
// =====================================================================================
void            LoadRadFiles( const char* const mapname, const char* const user_rad, const char* argv0 )
{
        char global_lights[_MAX_PATH];
        char mapname_lights[_MAX_PATH];

        char mapfile[_MAX_PATH];
        char mapdir[_MAX_PATH];
        char appdir[_MAX_PATH];

        // Get application directory (only an approximation on posix systems)
        // try looking in the directory we were run from
        {
                char tmp[_MAX_PATH];
                memset( tmp, 0, sizeof( tmp ) );
#ifdef _WIN32
                GetModuleFileName( NULL, tmp, _MAX_PATH );
#else
                safe_strncpy( tmp, argv0, _MAX_PATH );
#endif
                ExtractFilePath( tmp, appdir );
        }

        // Get map directory
        ExtractFilePath( mapname, mapdir );
        ExtractFile( mapname, mapfile );

        // Look for lights.rad in mapdir
        safe_strncpy( global_lights, mapdir, _MAX_PATH );
        safe_strncat( global_lights, lights_rad, _MAX_PATH );
        if ( q_exists( global_lights ) )
        {
                ReadLightFile( global_lights );
        }
        else
        {
                // Look for lights.rad in appdir
                safe_strncpy( global_lights, appdir, _MAX_PATH );
                safe_strncat( global_lights, lights_rad, _MAX_PATH );
                if ( q_exists( global_lights ) )
                {
                        ReadLightFile( global_lights );
                }
                else
                {
                        // Look for lights.rad in current working directory
                        safe_strncpy( global_lights, lights_rad, _MAX_PATH );
                        if ( q_exists( global_lights ) )
                        {
                                ReadLightFile( global_lights );
                        }
                }
        }

        // Look for mapname.rad in mapdir
        safe_strncpy( mapname_lights, mapdir, _MAX_PATH );
        safe_strncat( mapname_lights, mapfile, _MAX_PATH );
        safe_strncat( mapname_lights, ext_rad, _MAX_PATH );
        if ( q_exists( mapname_lights ) )
        {
                ReadLightFile( mapname_lights );
        }


        if ( user_rad )
        {
                char user_lights[_MAX_PATH];
                char userfile[_MAX_PATH];

                ExtractFile( user_rad, userfile );

                // Look for user.rad from command line (raw)
                safe_strncpy( user_lights, user_rad, _MAX_PATH );
                if ( q_exists( user_lights ) )
                {
                        ReadLightFile( user_lights );
                }
                else
                {
                        // Try again with .rad enforced as extension
                        DefaultExtension( user_lights, ext_rad );
                        if ( q_exists( user_lights ) )
                        {
                                ReadLightFile( user_lights );
                        }
                        else
                        {
                                // Look for user.rad in mapdir
                                safe_strncpy( user_lights, mapdir, _MAX_PATH );
                                safe_strncat( user_lights, userfile, _MAX_PATH );
                                DefaultExtension( user_lights, ext_rad );
                                if ( q_exists( user_lights ) )
                                {
                                        ReadLightFile( user_lights );
                                }
                                else
                                {
                                        // Look for user.rad in appdir
                                        safe_strncpy( user_lights, appdir, _MAX_PATH );
                                        safe_strncat( user_lights, userfile, _MAX_PATH );
                                        DefaultExtension( user_lights, ext_rad );
                                        if ( q_exists( user_lights ) )
                                        {
                                                ReadLightFile( user_lights );
                                        }
                                        else
                                        {
                                                // Look for user.rad in current working directory
                                                safe_strncpy( user_lights, userfile, _MAX_PATH );
                                                DefaultExtension( user_lights, ext_rad );
                                                if ( q_exists( user_lights ) )
                                                {
                                                        ReadLightFile( user_lights );
                                                }
                                        }
                                }
                        }
                }
        }

        ReadInfoTexlights(); // AJM
}

// =====================================================================================
//  main
// =====================================================================================
int             main( const int argc, char** argv )
{
        int             i;
        double          start, end;
        const char*     mapname_from_arg = NULL;
        const char*     user_lights = NULL;

        load_prc_file_data( "", "model-path /d/OTHER/lachb/Documents/cio/game/resources" );
        load_prc_file_data( "", "assert-abort 0" );
        load_prc_file_data( "", "load-file-type egg pandaegg" );
        load_prc_file_data( "", "egg-object-type-portal          <Scalar> portal { 1 }" );
        load_prc_file_data( "", "egg-object-type-polylight       <Scalar> polylight { 1 }" );
        load_prc_file_data( "", "egg-object-type-seq24           <Switch> { 1 } <Scalar> fps { 24 }" );
        load_prc_file_data( "", "egg-object-type-seq12           <Switch> { 1 } <Scalar> fps { 12 }" );
        load_prc_file_data( "", "egg-object-type-indexed         <Scalar> indexed { 1 }" );
        load_prc_file_data( "", "egg-object-type-seq10           <Switch> { 1 } <Scalar> fps { 10 }" );
        load_prc_file_data( "", "egg-object-type-seq8            <Switch> { 1 } <Scalar> fps { 8 }" );
        load_prc_file_data( "", "egg-object-type-seq6            <Switch> { 1 } <Scalar>  fps { 6 }" );
        load_prc_file_data( "", "egg-object-type-seq4            <Switch> { 1 } <Scalar>  fps { 4 }" );
        load_prc_file_data( "", "egg-object-type-seq2            <Switch> { 1 } <Scalar>  fps { 2 }" );
        load_prc_file_data( "", "egg-object-type-binary          <Scalar> alpha { binary }" );
        load_prc_file_data( "", "egg-object-type-dual            <Scalar> alpha { dual }" );
        load_prc_file_data( "", "egg-object-type-glass           <Scalar> alpha { blend_no_occlude }" );
        load_prc_file_data( "", "egg-object-type-model           <Model> { 1 }" );
        load_prc_file_data( "", "egg-object-type-dcs             <DCS> { 1 }" );
        load_prc_file_data( "", "egg-object-type-notouch         <DCS> { no_touch }" );
        load_prc_file_data( "", "egg-object-type-barrier         <Collide> { Polyset descend }" );
        load_prc_file_data( "", "egg-object-type-sphere          <Collide> { Sphere descend }" );
        load_prc_file_data( "", "egg-object-type-invsphere       <Collide> { InvSphere descend }" );
        load_prc_file_data( "", "egg-object-type-tube            <Collide> { Tube descend }" );
        load_prc_file_data( "", "egg-object-type-trigger         <Collide> { Polyset descend intangible }" );
        load_prc_file_data( "", "egg-object-type-trigger-sphere  <Collide> { Sphere descend intangible }" );
        load_prc_file_data( "", "egg-object-type-floor           <Collide> { Polyset descend level }" );
        load_prc_file_data( "", "egg-object-type-dupefloor       <Collide> { Polyset keep descend level }" );
        load_prc_file_data( "", "egg-object-type-bubble          <Collide> { Sphere keep descend }" );
        load_prc_file_data( "", "egg-object-type-ghost           <Scalar> collide-mask { 0 }" );
        load_prc_file_data( "", "egg-object-type-glow            <Scalar> blend { add }" );
        load_prc_file_data( "", "egg-object-type-direct-widget   <Scalar> collide-mask { 0x80000000 } <Collide> { Polyset descend }" );
        load_prc_file_data( "", "model-cache-dir" );
        load_prc_file_data( "", "model-cache-model #f" );
        load_prc_file_data( "", "model-cache-textures #f" );
        load_prc_file_data( "", "default-model-extension .egg" );
        //load_prc_file_data( "", "notify-level-raytrace debug" );
        //PStatClient::connect( "127.0.0.1" );
        if ( PStatClient::is_connected() )
        {
                PStatClient::main_tick();
        }

        RADTrace::initialize();

        g_Program = "p3rad";

        int argcold = argc;
        char ** argvold = argv;
        {
                int argc;
                char ** argv;
                ParseParamFile( argcold, argvold, argc, argv );
                {
                        if ( argc == 1 )
                                Usage();

                        for ( i = 1; i < argc; i++ )
                        {
                                if ( !strcasecmp( argv[i], "-dump" ) )
                                {
                                        g_dumppatches = true;
                                }
                                else if ( !strcasecmp( argv[i], "-bounce" ) )
                                {
                                        if ( i + 1 < argc )	//added "1" .--vluzacn
                                        {
                                                g_numbounce = atoi( argv[++i] );
                                                if ( g_numbounce > 1000 )
                                                {
                                                        Log( "Unexpectedly large value (>1000) for '-bounce'\n" );
                                                        Usage();
                                                }
                                        }
                                        else
                                        {
                                                Usage();
                                        }
                                }
                                /*
                                else if ( !strcasecmp( argv[i], "-mfincludefile" ) )
                                {
                                        if ( i + 1 < argc )
                                        {
                                                g_mfincludefile = argv[++i];
                                        }
                                        else
                                        {
                                                Warning( "i + 1 >= argc?\n" );
                                                Usage();
                                        }
                                }
                                */
                                else if ( !strcasecmp( argv[i], "-dev" ) )
                                {
                                        if ( i + 1 < argc )	//added "1" .--vluzacn
                                        {
                                                g_developer = (developer_level_t)atoi( argv[++i] );
                                        }
                                        else
                                        {
                                                Usage();
                                        }
                                }
                                else if ( !strcasecmp( argv[i], "-verbose" ) )
                                {
                                        g_verbose = true;
                                }
                                else if ( !strcasecmp( argv[i], "-noinfo" ) )
                                {
                                        g_info = false;
                                }
                                else if ( !strcasecmp( argv[i], "-mfdir" ) )
                                {
                                        if ( i + 1 < argc )
                                        {
                                                VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();

                                                std::string mfdir( argv[++i] );
                                                PT( VirtualFileList ) list = vfs->scan_directory( mfdir );

                                                if ( !list )
                                                {
                                                        Warning( "%s is not a valid directory, no multifiles mounted", mfdir.c_str() );
                                                        continue;
                                                }

                                                load_prc_file_data( "", "model-path " + Filename::from_os_specific( mfdir ).get_fullpath() );

                                                for ( size_t filenum = 0; filenum < list->get_num_files(); filenum++ )
                                                {
                                                        Filename fn = list->get_file( filenum )->get_filename();

                                                        if ( fn.get_extension() != "mf" )
                                                                continue;

                                                        Log( "Mounting multifile %s\n", fn.get_fullpath().c_str() );
                                                        VirtualFileSystem::get_global_ptr()->mount( fn,
                                                                                                    Filename( "." ),
                                                                                                    VirtualFileSystem::MF_read_only );
                                                }
                                        }
                                        else
                                        {
                                                Usage();
                                        }
                                }
                                else if ( !strcasecmp( argv[i], "-threads" ) )
                                {
                                        if ( i + 1 < argc )	//added "1" .--vluzacn
                                        {
                                                g_numthreads = atoi( argv[++i] );
                                                if ( g_numthreads < 1 )
                                                {
                                                        Log( "Expected value of at least 1 for '-threads'\n" );
                                                        Usage();
                                                }
                                        }
                                        else
                                        {
                                                Usage();
                                        }
                                }
#ifdef _WIN32
                                else if ( !strcasecmp( argv[i], "-estimate" ) )
                                {
                                        g_estimate = true;
                                }
#endif
#ifdef SYSTEM_POSIX
                                else if ( !strcasecmp( argv[i], "-noestimate" ) )
                                {
                                        g_estimate = false;
                                }
#endif
#ifdef ZHLT_NETVIS
                                else if ( !strcasecmp( argv[i], "-client" ) )
                                {
                                        if ( i + 1 < argc )	//added "1" .--vluzacn
                                        {
                                                g_clientid = atoi( argv[++i] );
                                        }
                                        else
                                        {
                                                Usage();
                                        }
                                }
#endif
                                else if ( !strcasecmp( argv[i], "-fast" ) )
                                {
                                        g_fastmode = true;
                                }
                                else if ( !strcasecmp( argv[i], "-notexscale" ) )
                                {
                                        g_texscale = false;
                                }
                                else if ( !strcasecmp( argv[i], "-ambient" ) )
                                {
                                        if ( i + 3 < argc )
                                        {
                                                g_ambient[0] = (float)atof( argv[++i] ) * 128;
                                                g_ambient[1] = (float)atof( argv[++i] ) * 128;
                                                g_ambient[2] = (float)atof( argv[++i] ) * 128;
                                        }
                                        else
                                        {
                                                Error( "expected three color values after '-ambient'\n" );
                                        }
                                }
                                else if ( !strcasecmp( argv[i], "-incremental" ) )
                                {
                                        g_incremental = true;
                                }
                                else if ( !strcasecmp( argv[i], "-chart" ) )
                                {
                                        g_chart = true;
                                }
                                else if ( !strcasecmp( argv[i], "-low" ) )
                                {
                                        g_threadpriority = TP_low;
                                }
                                else if ( !strcasecmp( argv[i], "-high" ) )
                                {
                                        g_threadpriority = TP_high;
                                }
                                else if ( !strcasecmp( argv[i], "-nolog" ) )
                                {
                                        g_log = false;
                                }
                                else if ( !strcasecmp( argv[i], "-dlight" ) )
                                {
                                        if ( i + 1 < argc )	//added "1" .--vluzacn
                                        {
                                                g_dlight_threshold = (float)atof( argv[++i] );
                                        }
                                        else
                                        {
                                                Usage();
                                        }
                                }
                                else if ( !strcasecmp( argv[i], "-extra" ) )
                                {
                                        g_extra = true;
                                }
                                else if ( !strcasecmp( argv[i], "-final" ) )
                                {
                                        g_skysamplescale = 16.0;
                                }
                                else if ( !strcasecmp( argv[i], "-smooth" ) )
                                {
                                        if ( i + 1 < argc )	//added "1" .--vluzacn
                                        {
                                                g_smoothing_value = atof( argv[++i] );
                                        }
                                        else
                                        {
                                                Usage();
                                        }
                                }
                                else if ( !strcasecmp( argv[i], "-dscale" ) )
                                {
                                        if ( i + 1 < argc )	//added "1" .--vluzacn
                                        {
                                                g_direct_scale = (float)atof( argv[++i] );
                                        }
                                        else
                                        {
                                                Usage();
                                        }
                                }
                                else if ( !strcasecmp( argv[i], "-depth" ) )
                                {
                                        if ( i + 1 < argc )
                                        {
                                                g_translucentdepth = atof( argv[++i] );
                                        }
                                        else
                                        {
                                                Usage();
                                        }
                                }
                                else if ( !strcasecmp( argv[i], "-notextures" ) )
                                {
                                        g_notextures = true;
                                }
                                else if ( !strcasecmp( argv[i], "-texreflectscale" ) )
                                {
                                        if ( i + 1 < argc )
                                        {
                                                g_texreflectscale = atof( argv[++i] );
                                        }
                                        else
                                        {
                                                Usage();
                                        }
                                }

                                else if ( argv[i][0] == '-' )
                                {
                                        Log( "Unknown option \"%s\"\n", argv[i] );
                                        Usage();
                                }
                                else if ( !mapname_from_arg )
                                {
                                        mapname_from_arg = argv[i];
                                }
                                else
                                {
                                        Log( "Unknown option \"%s\"\n", argv[i] );
                                        Usage();
                                }
                        }

                        if ( !mapname_from_arg )
                        {
                                Log( "No mapname specified\n" );
                                Usage();
                        }

                        g_smoothing_threshold = (float)cos( g_smoothing_value * ( Q_PI / 180.0 ) );

                        safe_strncpy( g_Mapname, mapname_from_arg, _MAX_PATH );
                        FlipSlashes( g_Mapname );
                        StripExtension( g_Mapname );
                        OpenLog( g_clientid );
                        atexit( CloseLog );
                        ThreadSetDefault();
                        ThreadSetPriority( g_threadpriority );
                        LogStart( argcold, argvold );
                        {
                                int			 i;
                                Log( "Arguments: " );
                                for ( i = 1; i < argc; i++ )
                                {
                                        if ( strchr( argv[i], ' ' ) )
                                        {
                                                Log( "\"%s\" ", argv[i] );
                                        }
                                        else
                                        {
                                                Log( "%s ", argv[i] );
                                        }
                                }
                                Log( "\n" );
                        }

                        CheckForErrorLog();

#ifdef PLATFORM_CAN_CALC_EXTENT
                        hlassume( CalcFaceExtents_test(), assume_first );
#endif
                        dtexdata_init();
                        atexit( dtexdata_free );

                        //ReadMFIncludeFile();

                        // END INIT

                        // BEGIN RAD
                        start = I_FloatTime();

                        // normalise maxlight

                        safe_snprintf( g_source, _MAX_PATH, "%s.bsp", g_Mapname );
                        g_bspdata = LoadBSPFile( g_source );
#ifndef PLATFORM_CAN_CALC_EXTENT
                        char extentfilename[_MAX_PATH];
                        safe_snprintf( extentfilename, _MAX_PATH, "%s.ext", g_Mapname );
                        Log( "Loading extent file '%s'\n", extentfilename );
                        if ( !q_exists( extentfilename ) )
                        {
                                hlassume( false, assume_NO_EXTENT_FILE );
                        }
                        LoadExtentFile( extentfilename );
#endif
                        ParseEntities( g_bspdata );
                        if ( g_fastmode )
                        {
                                g_numbounce = 0;
                                g_softsky = false;
                        }
                        Settings();

                        VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();
                        for ( size_t i = 0; i < g_multifiles.size(); i++ )
                        {
                                Filename file = Filename::from_os_specific( g_multifiles[i] );
                                if ( !vfs->mount( file, ".", VirtualFileSystem::MF_read_only ) )
                                {
                                        Warning( "Could not mount multifile from %s!\n", file.get_fullpath().c_str() );
                                }
                                else
                                {
                                        Log( "Mounted multifile %s.\n", file.get_fullpath().c_str() );
                                }
                        }


                        g_face_patches.resize( MAX_MAP_FACES );
                        g_face_parents.resize( MAX_MAP_FACES );
                        g_cluster_children.resize( MAX_MAP_LEAFS );

                        for ( int i = 0; i < MAX_MAP_FACES; i++ )
                        {
                                g_face_patches[i] = -1;
                                g_face_parents[i] = -1;
                        }
                        for ( int i = 0; i < MAX_MAP_LEAFS; i++ )
                        {
                                g_cluster_children[i] = -1;
                        }

                        LoadStaticProps();
                        LoadTextures();
                        LoadRadFiles( g_Mapname, user_lights, argv[0] );
                        ReadCustomSmoothValue();
                        ReadTranslucentTextures();
                        ReadLightingCone();
                        g_smoothing_threshold_2 = g_smoothing_value_2 < 0 ? g_smoothing_threshold : (float)cos( g_smoothing_value_2 * ( Q_PI / 180.0 ) );
                        {
                                int style;
                                for ( style = 0; style < ALLSTYLES; ++style )
                                {
                                        g_corings[style] = style ? g_coring : 0;
                                }
                        }
                        //if ( g_direct_scale != 1.0 )
                        //{
                        //        Warning( "dscale value should be 1.0 for final compile.\nIf you need to adjust the bounced light, use the '-texreflectscale' and '-texreflectgamma' options instead." );
                        //}
                        //if ( g_colour_lightscale[0] != 2.0 || g_colour_lightscale[1] != 2.0 || g_colour_lightscale[2] != 2.0 )
                        //{
                        //        Warning( "light scale value should be 2.0 for final compile.\nValues other than 2.0 will result in incorrect interpretation of light_environment's brightness when the engine loads the map." );
                        //}
                        if ( g_drawlerp )
                        {
                                g_direct_scale = 0.0;
                        }

                        if ( !g_bspdata->visdatasize )
                        {
                                Warning( "No vis information." );
                        }
                        if ( g_blur < 1.0 )
                        {
                                g_blur = 1.0;
                        }

                        RadWorld();

                        if ( g_chart )
                                PrintBSPFileSizes( g_bspdata );

                        WriteBSPFile( g_bspdata, g_source );

                        end = I_FloatTime();
                        LogTimeElapsed( end - start );
                        // END RAD

                        

                        if ( PStatClient::is_connected() )
                        {
                                PStatClient::main_tick();
                                PStatClient::main_tick();
                                system( "pause" );
                        }
                }
        }
        return 0;
}

/*
void ReadMFIncludeFile()
{
        Log( "Multifile include file is %s.\n", g_mfincludefile.c_str() );

        char *buffer;
        LoadFile( g_mfincludefile.c_str(), &buffer );
        string strbuf = buffer;

        vector<string> lines = explode( "\n", strbuf );
        for ( size_t linenum = 0; linenum < lines.size(); linenum++ )
        {
                string mfpath = lines[linenum];
                if ( linenum < lines.size() - 1 )
                {
                        mfpath = mfpath.substr( 0, mfpath.length() - 1 );
                }
                g_multifiles.push_back( mfpath );
                Log( "%s will attempt to be mounted.\n", mfpath.c_str() );
        }

        delete buffer;
}
*/