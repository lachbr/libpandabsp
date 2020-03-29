#ifndef RESOURCELOCK_H__
#define RESOURCELOCK_H__
#include "cmdlib.h" //--vluzacn

#if _MSC_VER >= 1000
#pragma once
#endif

extern _BSPEXPORT void*    CreateResourceLock(int LockNumber);
extern _BSPEXPORT void     ReleaseResourceLock(void** lock);

#endif // RESOURCE_LOCK_H__
