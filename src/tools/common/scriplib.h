#ifndef SCRIPLIB_H__
#define SCRIPLIB_H__

#if _MSC_VER >= 1000
#pragma once
#endif

#include "cmdlib.h"

#define	MAXTOKEN 4096

extern _BSPEXPORT char     g_token[MAXTOKEN];
extern _BSPEXPORT char     g_TXcommand;                               // global for Quark maps texture alignment hack

extern _BSPEXPORT void     LoadScriptFile( const char* const filename );
extern _BSPEXPORT void     ParseFromMemory( char* buffer, int size );

extern _BSPEXPORT bool     GetToken( bool crossline );
extern _BSPEXPORT void     UnGetToken();
extern _BSPEXPORT bool     TokenAvailable();

#define MAX_WAD_PATHS   42
extern _BSPEXPORT char         g_szWadPaths[MAX_WAD_PATHS][_MAX_PATH];
extern _BSPEXPORT int          g_iNumWadPaths;

#endif //**/ SCRIPLIB_H__
