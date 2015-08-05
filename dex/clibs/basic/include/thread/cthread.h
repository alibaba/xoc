/*Copyright 2011 Alibaba Group*/
/*
 *  cthread.h
 *  madagascar
 *
 *  Created by Aaron Wang on 4/15/09.
 *  Copyright 2009 Virtualnology. All rights reserved.
 *
 */

#ifndef CLIBS_THREAD_H
#define CLIBS_THREAD_H

#include "std/cstd.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct STMutex {
    volatile void* mutex;
    volatile Int32 cnt;
    volatile UInt32 tid;
} MutexT;

EXTERN_C void cSleep(Int32 ms);

EXTERN_C void cMutexInit(MutexT* mt);

EXTERN_C void cMutexLock(MutexT* mt);

EXTERN_C void cMutexUnlock(MutexT* mt);

EXTERN_C void cMutexDestroy(MutexT* mt);

#ifdef __cplusplus
}
#endif


#endif
