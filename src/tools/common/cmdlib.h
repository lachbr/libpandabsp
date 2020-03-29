#ifndef CMDLIB_H__
#define CMDLIB_H__

#pragma warning(disable: 4251)

#include "common_config.h"

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
using std::string;
using std::vector;
using std::map;
using std::cout;
using std::cerr;
using std::endl;

#if _MSC_VER >= 1000
#pragma once
#endif

//#define MODIFICATIONS_STRING "Submit detailed bug reports to (zoner@gearboxsoftware.com)\n"
//#define MODIFICATIONS_STRING "Submit detailed bug reports to (merlinis@bigpond.net.au)\n"
//#define MODIFICATIONS_STRING "Submit detailed bug reports to (amckern@yahoo.com)\n"
//#define MODIFICATIONS_STRING "Submit detailed bug reports to (vluzacn@163.com)\n" //--vluzacn
#define MODIFICATIONS_STRING "Submit detailed bug reports to (brianlach72@gmail.com)\n"

#ifdef _DEBUG
#define TOOLS_VERSIONSTRING "v1.0 dbg"
#else
#define TOOLS_VERSIONSTRING "v1.0"
#endif

//#if !defined (HLCSG) && !defined (HLBSP) && !defined (HLVIS) && !defined (HLRAD) && !defined (RIPENT) //--vluzacn
//#error "You must define one of these in the settings of each project: HLCSG, HLBSP, HLVIS, HLRAD, RIPENT. The most likely cause is that you didn't load the project from the sln file."
//#endif
#if !defined (VERSION_32BIT) && !defined (VERSION_64BIT) && !defined (VERSION_LINUX) && !defined (VERSION_OTHER) //--vluzacn
#error "You must define one of these in the settings of each project: VERSION_32BIT, VERSION_64BIT, VERSION_LINUX, VERSION_OTHER. The most likely cause is that you didn't load the project from the sln file."
#endif

#ifdef VERSION_32BIT
#define PLATFORM_VERSIONSTRING "32-bit"
#define PLATFORM_CAN_CALC_EXTENT
#endif
#ifdef VERSION_64BIT
#define PLATFORM_VERSIONSTRING "64-bit"
#define PLATFORM_CAN_CALC_EXTENT
#endif
#ifdef VERSION_LINUX
#define PLATFORM_VERSIONSTRING "linux"
#define PLATFORM_CAN_CALC_EXTENT
#endif
#ifdef VERSION_OTHER
#define PLATFORM_VERSIONSTRING "???"
#endif

//=====================================================================
// AJM: Different features of the tools can be undefined here
//      these are not officially beta tested, but seem to work okay

// ZHLT_* features are spread across more than one tool. Hence, changing
//      one of these settings probably means recompiling the whole set
//#define ZHLT_DETAIL                         // HLCSG, HLBSP - detail brushes     //should never turn on
//#define ZHLT_PROGRESSFILE                   // ALL TOOLS - estimate progress reporting to -progressfile //should never turn on
//#define ZHLT_NSBOB //should never turn on
//#define ZHLT_CONSOLE //--vluzacn
//#define ZHLT_XASH // build the compiler for Xash engine //--vluzacn
//	#ifdef ZHLT_64BIT_FIX
//#define ZHLT_EMBEDLIGHTMAP // this feature requires HLRAD_TEXTURE and RIPENT_TEXTURE //--vluzacn
//	#endif
//#define ZHLT_HIDDENSOUNDTEXTURE //--vluzacn


#ifdef _WIN32
#define RIPENT_PAUSE //--vluzacn
#endif
#define RIPENT_TEXTURE //--vluzacn

// tool specific settings below only mean a recompile of the tool affected


#ifdef _WIN32
#define HLCSG_GAMETEXTMESSAGE_UTF8 //--vluzacn
#endif
//#define HLBSP_SUBDIVIDE_INMID // this may contribute to 'AllocBlock: full' problem though it may generate fewer faces. --vluzacn




//error : inserted by sunifdef: "#define HLRAD_CUSTOMCHOP // don't use this --vluzacn" contradicts -U at D:\OTHER\lachb\Documents\rpbmStudios\panda_hl_bsp_tools\common\cmdlib.h(342)
//error : inserted by sunifdef: "#define HLRAD_OPAQUE_GROUP //--vluzacn //obsolete" contradicts -U at D:\OTHER\lachb\Documents\rpbmStudios\panda_hl_bsp_tools\common\cmdlib.h(358)

//=====================================================================

#if _MSC_VER <1400
#define strcpy_s strcpy //--vluzacn
#define sprintf_s sprintf //--vluzacn
#endif
#if _MSC_VER >= 1400
#pragma warning(disable: 4996)
#endif

#ifdef __MINGW32__
#include <io.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if 0 //--vluzacn
// AJM: gnu compiler fix
#ifdef __GNUC__
#define _alloca __builtin_alloca
#define alloca __builtin_alloca
#endif
#endif

#include "win32fix.h"
#include "mathtypes.h"

#ifdef _WIN32
#pragma warning(disable: 4127)                      // conditional expression is constant
#pragma warning(disable: 4115)                      // named type definition in parentheses
#pragma warning(disable: 4244)                      // conversion from 'type' to type', possible loss of data
// AJM
#pragma warning(disable: 4786)                      // identifier was truncated to '255' characters in the browser information
#pragma warning(disable: 4305)                      // truncation from 'const double' to 'float'
#pragma warning(disable: 4800)                     // forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable: 4005)
#endif


#ifdef STDC_HEADERS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <limits.h>
#endif

#include <stdint.h> //--vluzacn

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef ZHLT_NETVIS
#include "c2cpp.h"
#endif

#ifdef _WIN32
#define SYSTEM_SLASH_CHAR  '\\'
#define SYSTEM_SLASH_STR   "\\"
#endif
#ifdef SYSTEM_POSIX
#define SYSTEM_SLASH_CHAR  '/'
#define SYSTEM_SLASH_STR   "/"
#endif

// the dec offsetof macro doesn't work very well...
#define myoffsetof(type,identifier) ((size_t)&((type*)0)->identifier)
#define sizeofElement(type,identifier) (sizeof((type*)0)->identifier)

#ifdef SYSTEM_POSIX
extern _BSPEXPORT char*    strupr( char* string );
extern _BSPEXPORT char*    strlwr( char* string );
#endif
extern _BSPEXPORT const char* stristr( const char* const string, const char* const substring );
extern _BSPEXPORT bool CDECL FORMAT_PRINTF( 3, 4 ) safe_snprintf( char* const dest, const size_t count, const char* const args, ... );
extern _BSPEXPORT bool     safe_strncpy( char* const dest, const char* const src, const size_t count );
extern _BSPEXPORT bool     safe_strncat( char* const dest, const char* const src, const size_t count );
extern _BSPEXPORT bool     TerminatedString( const char* buffer, const int size );

extern _BSPEXPORT char*    FlipSlashes( char* string );

extern _BSPEXPORT double   I_FloatTime();

extern _BSPEXPORT int      CheckParm( char* check );

extern _BSPEXPORT void     DefaultExtension( char* path, const char* extension );
extern _BSPEXPORT void     DefaultPath( char* path, char* basepath );
extern _BSPEXPORT void     StripFilename( char* path );
extern _BSPEXPORT void     StripExtension( char* path );

extern _BSPEXPORT void     ExtractFile( const char* const path, char* dest );
extern _BSPEXPORT void     ExtractFilePath( const char* const path, char* dest );
extern _BSPEXPORT void     ExtractFileBase( const char* const path, char* dest );
extern _BSPEXPORT void     ExtractFileExtension( const char* const path, char* dest );

extern _BSPEXPORT short    BigShort( short l );
extern _BSPEXPORT short    LittleShort( short l );
extern _BSPEXPORT int      BigLong( int l );
extern _BSPEXPORT int      LittleLong( int l );
extern _BSPEXPORT float    BigFloat( float l );
extern _BSPEXPORT float    LittleFloat( float l );

extern _BSPEXPORT bool strcontains( const string &str, const string &keyword );
extern _BSPEXPORT vector<string> explode( const string &delimiter, const string &str );

#endif //CMDLIB_H__
