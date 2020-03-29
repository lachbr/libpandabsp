#ifndef BLOCKMEM_H__
#define BLOCKMEM_H__
#include "cmdlib.h" //--vluzacn

#if _MSC_VER >= 1000
#pragma once
#endif

extern _BSPEXPORT void*    AllocBlock( unsigned long size );
extern _BSPEXPORT bool     FreeBlock( void* pointer );

extern _BSPEXPORT void*    Alloc( unsigned long size );
extern _BSPEXPORT bool     Free( void* pointer );

#if defined(CHECK_HEAP)
extern _BSPEXPORT void     HeapCheck();
#else
#define HeapCheck()
#endif

#endif // BLOCKMEM_H__
