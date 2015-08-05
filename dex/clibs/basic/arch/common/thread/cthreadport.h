/*Copyright 2011 Alibaba Group*/
/*
 *  cthread.h
 *  madagascar
 *
 *  Created by Aaron Wang on 4/15/09.
 *  Copyright 2009 Virtualnology. All rights reserved.
 *
 */

#ifndef CLIBS_THREAD_PORT_H
#define CLIBS_THREAD_PORT_H

#include "thread/cthread.h"


EXTERN_C void cSleepPort(Int32 ms);

EXTERN_C void cMutexInitPort(MutexT* mt);

EXTERN_C void cMutexLockPort(MutexT* mt);

EXTERN_C void cMutexUnlockPort(MutexT* mt);

EXTERN_C void cMutexDestroyPort(MutexT* mt);


#endif
