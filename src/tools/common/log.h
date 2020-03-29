#ifndef LOG_H__
#define LOG_H__
#include "cmdlib.h" //--vluzacn

#if _MSC_VER >= 1000
#pragma once
#endif

#include "mathtypes.h"
#include "messages.h"

typedef enum
{
        DEVELOPER_LEVEL_ALWAYS,
        DEVELOPER_LEVEL_ERROR,
        DEVELOPER_LEVEL_WARNING,
        DEVELOPER_LEVEL_MESSAGE,
        DEVELOPER_LEVEL_FLUFF,
        DEVELOPER_LEVEL_SPAM,
        DEVELOPER_LEVEL_MEGASPAM
}
developer_level_t;

//
// log.c globals
//

extern _BSPEXPORT char*    g_Program;
extern _BSPEXPORT char     g_Mapname[_MAX_PATH];

#define DEFAULT_DEVELOPER   DEVELOPER_LEVEL_ALWAYS
#define DEFAULT_VERBOSE     false
#define DEFAULT_LOG         true

extern _BSPEXPORT developer_level_t g_developer;
extern _BSPEXPORT bool          g_verbose;
extern _BSPEXPORT bool          g_log;
extern _BSPEXPORT unsigned long g_clientid;                           // Client id of this program
extern _BSPEXPORT unsigned long g_nextclientid;                       // Client id of next client to spawn from this server

                                                           //
                                                           // log.c Functions
                                                           //

extern _BSPEXPORT void     ResetTmpFiles();
extern _BSPEXPORT void     ResetLog();
extern _BSPEXPORT void     ResetErrorLog();
extern _BSPEXPORT void     CheckForErrorLog();

extern _BSPEXPORT void CDECL OpenLog( int clientid );
extern _BSPEXPORT void CDECL CloseLog();
extern _BSPEXPORT void     WriteLog( const char* const message );

extern _BSPEXPORT void     CheckFatal();

extern _BSPEXPORT void CDECL FORMAT_PRINTF( 2, 3 ) Developer( developer_level_t level, const char* const message, ... );

#ifdef _DEBUG
#define IfDebug(x) (x)
#else
#define IfDebug(x)
#endif

extern _BSPEXPORT const char * Localize( const char *s );
extern _BSPEXPORT void LoadLangFile( const char *name, const char *programpath );
extern _BSPEXPORT void CDECL FORMAT_PRINTF( 1, 2 ) Verbose( const char* const message, ... );
extern _BSPEXPORT void CDECL FORMAT_PRINTF( 1, 2 ) Log( const char* const message, ... );
extern _BSPEXPORT void CDECL FORMAT_PRINTF( 1, 2 ) Error( const char* const error, ... );
extern _BSPEXPORT void CDECL FORMAT_PRINTF( 2, 3 ) Fatal( assume_msgs msgid, const char* const error, ... );
extern _BSPEXPORT void CDECL FORMAT_PRINTF( 1, 2 ) Warning( const char* const warning, ... );

extern _BSPEXPORT void CDECL FORMAT_PRINTF( 1, 2 ) PrintOnce( const char* const message, ... );

extern _BSPEXPORT void     LogStart( const int argc, char** argv );
extern _BSPEXPORT void     LogEnd();
extern _BSPEXPORT void     Banner();

extern _BSPEXPORT void     LogTimeElapsed( float elapsed_time );

// Should be in hlassert.h, but well so what
extern _BSPEXPORT void     hlassume( bool exp, assume_msgs msgid );

#endif // Should be in hlassert.h, but well so what LOG_H__
