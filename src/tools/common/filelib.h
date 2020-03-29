#ifndef FILELIB_H__
#define FILELIB_H__
#include "cmdlib.h" //--vluzacn

#if _MSC_VER >= 1000
#pragma once
#endif

extern _BSPEXPORT time_t   getfiletime( const char* const filename );
extern _BSPEXPORT long     getfilesize( const char* const filename );
extern _BSPEXPORT long     getfiledata( const char* const filename, char* buffer, const int buffersize );
extern _BSPEXPORT bool     q_exists( const char* const filename );
extern _BSPEXPORT int      q_filelength( FILE* f );

extern _BSPEXPORT FILE*    SafeOpenWrite( const char* const filename );
extern _BSPEXPORT FILE*    SafeOpenRead( const char* const filename );
extern _BSPEXPORT void     SafeRead( FILE* f, void* buffer, int count );
extern _BSPEXPORT void     SafeWrite( FILE* f, const void* const buffer, int count );

extern _BSPEXPORT int      LoadFile( const char* const filename, char** bufferptr );
extern _BSPEXPORT void     SaveFile( const char* const filename, const void* const buffer, int count );

#endif //**/ FILELIB_H__
