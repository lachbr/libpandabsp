#ifndef THREADS_H__
#define THREADS_H__
#include "cmdlib.h" //--vluzacn

#include <pointerTo.h>
#include <lightMutex.h>
#include <genericThread.h>
#include <threadPriority.h>
#include <pvector.h>

#if _MSC_VER >= 1000
#pragma once
#endif

#define	MAX_THREADS	64

typedef void q_threadfunction( int );

#ifdef _WIN32
#define DEFAULT_NUMTHREADS -1
#endif
#ifdef SYSTEM_POSIX
#define DEFAULT_NUMTHREADS 1
#endif

class _BSPEXPORT BSPThread : public Thread
{
public:
        BSPThread();
        void set_function( q_threadfunction *func );
        void set_value( int val );
        volatile bool is_finished() const;

protected:
        virtual void thread_main();

private:
        q_threadfunction * _func;
        int _val;
        volatile bool _finished;
};

#define DEFAULT_THREAD_PRIORITY TP_normal

extern _BSPEXPORT int      g_numthreads;
extern _BSPEXPORT ThreadPriority g_threadpriority;
extern _BSPEXPORT LightMutex g_global_lock;
extern _BSPEXPORT pvector<PT( BSPThread )> g_threadhandles;

extern _BSPEXPORT int	GetCurrentThreadNumber();

extern _BSPEXPORT void     ThreadSetPriority( ThreadPriority type );
extern _BSPEXPORT void     ThreadSetDefault();
extern _BSPEXPORT int      GetThreadWork();
extern _BSPEXPORT void     ThreadLock();
extern _BSPEXPORT void     ThreadUnlock();

extern _BSPEXPORT void     RunThreadsOnIndividual( int workcnt, bool showpacifier, q_threadfunction );
extern _BSPEXPORT void     RunThreadsOn( int workcnt, bool showpacifier, q_threadfunction );

#ifdef ZHLT_NETVIS
extern _BSPEXPORT void     threads_InitCrit();
extern _BSPEXPORT void     threads_UninitCrit();
#endif

#define NamedRunThreadsOn(n,p,f) { printf("%-20s ", #f ":"); RunThreadsOn(n,p,f); }
#define NamedRunThreadsOnIndividual(n,p,f) { printf("%-20s ", #f ":"); RunThreadsOnIndividual(n,p,f); }

#endif //**/ THREADS_H__
